#include "Components/URiftEquipmentComponent.h"
#include "Components/URiftInventoryComponent.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "GameFramework/Fragments/URiftFragment_Equippable.h"
#include "GameFramework/Fragments/URiftFragment_Equippable_GAS.h"
#include "GameFramework/Fragments/URiftFragment_Equippable_Weapon.h"
#include "GameFramework/Containers/URiftContainer.h"
#include "Actors/ARiftWeaponActor.h"
#include "Abilities/URiftAbility_Equip.h"
#include "Abilities/URiftAbility_Unequip.h"
#include "Abilities/URiftAbility_Drop.h"
#include "Data/URiftItemDefinition.h"
#include "Tags/RiftVaultTags.h"
#include "Interfaces/IRiftInventoryInterface.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "Engine/ActorChannel.h"
#include "Net/UnrealNetwork.h"
#include "Components/SkeletalMeshComponent.h"

// --------------------------------------------------------------------
// Construction
// --------------------------------------------------------------------

URiftEquipmentComponent::URiftEquipmentComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // This component must replicate so EquipmentSlots reaches clients.
    SetIsReplicatedByDefault(true);

    // Equipment management is event-driven; ticking would waste CPU every frame.
    PrimaryComponentTick.bCanEverTick = false;

    // Set the back-pointer now so FastArray callbacks always have a valid OwnerComponent,
    // even if they fire very early (before BeginPlay).
    EquipmentSlots.OwnerComponent = this;
}

// --------------------------------------------------------------------
// Actor lifecycle
// --------------------------------------------------------------------

void URiftEquipmentComponent::BeginPlay()
{
    Super::BeginPlay();

    // The server runs OnPawnReady to link the inventory and grant abilities.
    // The owning client also runs it so LinkedInventory is populated — this is
    // required for TryResolveEquipmentSlotItem to look up items in inventory
    // containers when PostReplicatedAdd fires. Non-owning (simulated) clients
    // skip this entirely; they have no inventory replication and only need the
    // ItemDefinition pointer that travels in the equipment slot entry.
    APawn* OwningPawn = GetOwner<APawn>();
    if (OwningPawn && (OwningPawn->HasAuthority() || OwningPawn->IsLocallyControlled()))
    {
        OnPawnReady();
    }
}

void URiftEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Destroy every weapon actor that was spawned by this component.
    // This handles all end-play scenarios: pawn death, level transition, PIE stop, etc.
    for (auto& Pair : SpawnedWeaponActors)
    {
        if (Pair.Value) { Pair.Value->Destroy(); }
    }
    SpawnedWeaponActors.Empty();

    for (auto& Pair : SpawnedOffHandActors)
    {
        if (Pair.Value) { Pair.Value->Destroy(); }
    }
    SpawnedOffHandActors.Empty();

    // Clear the ability set handle bookkeeping map.
    // Abilities, effects, and attribute sets are cleaned up by the ASC when the actor is
    // destroyed; this just prevents stale handle references from lingering on the component.
    GrantedAbilitySetHandles.Empty();

    Super::EndPlay(EndPlayReason);
}

// --------------------------------------------------------------------
// Replication
// --------------------------------------------------------------------

void URiftEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Replicate the slot list to all clients using Fast TArray delta serialization.
    // FRiftEquipmentSlotList::NetDeltaSerialize handles the per-entry diff logic.
    DOREPLIFETIME(URiftEquipmentComponent, EquipmentSlots);
}

bool URiftEquipmentComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
    bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

    // Item instances are owned by URiftInventoryComponent on APlayerState and are
    // replicated there. EquipmentSlots entries only carry object pointers, so no
    // additional sub-object replication is needed from this component.
    return bWroteSomething;
}

// --------------------------------------------------------------------
// Equipment API
// --------------------------------------------------------------------

bool URiftEquipmentComponent::EquipItem(URiftItemInstance* Item, FGameplayTag SlotTag)
{
    if (!Item || !SlotTag.IsValid())
    {
        return false;
    }

    if (!IsSlotSupported(SlotTag))
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftEquipmentComponent::EquipItem — slot %s is not supported on this pawn."), *SlotTag.ToString());
        return false;
    }

    if (IsSlotOccupied(SlotTag))
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftEquipmentComponent::EquipItem — slot %s is already occupied."), *SlotTag.ToString());
        return false;
    }

    const URiftFragment_Equippable* Fragment = Item->FindFragment<URiftFragment_Equippable>();
    if (!Fragment)
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftEquipmentComponent::EquipItem — item has no URiftFragment_Equippable."));
        return false;
    }

    if (Fragment->GetEquipmentSlotTag() != SlotTag)
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftEquipmentComponent::EquipItem — item's slot tag (%s) does not match target slot (%s)."),
            *Fragment->GetEquipmentSlotTag().ToString(), *SlotTag.ToString());
        return false;
    }

    // FinishEquip records the slot entry BEFORE moving the item. This means that when
    // MoveItemToContainer fires OnItemMoved below, IsSlotOccupied will already return
    // true and OnInventoryItemMoved will skip re-processing — preventing double equip.
    FinishEquip(Item, SlotTag);

    // Move the item into its per-slot container so the inventory's state matches.
    if (LinkedInventory.IsValid())
    {
        if (URiftContainer* SlotContainer = LinkedInventory->GetContainerByTag(SlotTag))
        {
            LinkedInventory->MoveItemToContainer(Item, SlotContainer);
        }
    }

    return true;
}

bool URiftEquipmentComponent::UnequipItem(FGameplayTag SlotTag)
{
    FRiftEquipmentSlotEntry* Entry = EquipmentSlots.FindEntryForSlot(SlotTag);
    if (!Entry || !Entry->Item)
    {
        return false;
    }

    URiftItemInstance* Item = Entry->Item;

    // FinishUnequip removes the slot entry BEFORE moving the item. This means that when
    // MoveItemToContainer fires OnItemMoved below, IsSlotOccupied will already return
    // false and OnInventoryItemMoved will skip re-processing — preventing double unequip.
    FinishUnequip(Item, SlotTag);

    // Return the item to the backpack container.
    if (LinkedInventory.IsValid())
    {
        if (URiftContainer* BackpackContainer = LinkedInventory->GetContainerByTag(Tag_Rift_Container_Backpack))
        {
            LinkedInventory->MoveItemToContainer(Item, BackpackContainer);
        }
    }

    return true;
}

// --------------------------------------------------------------------
// Query API
// --------------------------------------------------------------------

URiftItemInstance* URiftEquipmentComponent::GetItemInSlot(FGameplayTag SlotTag) const
{
    // Delegate to the slot list's linear search; returns nullptr if slot is empty.
    const FRiftEquipmentSlotEntry* Entry = EquipmentSlots.FindEntryForSlot(SlotTag);
    return Entry ? Entry->Item : nullptr;
}

bool URiftEquipmentComponent::IsSlotOccupied(FGameplayTag SlotTag) const
{
    // A slot is occupied if GetItemInSlot returns a non-null pointer.
    return GetItemInSlot(SlotTag) != nullptr;
}

TArray<URiftItemInstance*> URiftEquipmentComponent::GetAllEquippedItems() const
{
    TArray<URiftItemInstance*> Result;
    for (const FRiftEquipmentSlotEntry& Entry : EquipmentSlots.Entries)
    {
        // Skip any entries whose item pointer is null (should not happen in normal operation
        // but defensive null-check avoids returning garbage to callers).
        if (Entry.Item)
        {
            Result.Add(Entry.Item);
        }
    }
    return Result;
}

bool URiftEquipmentComponent::IsSlotSupported(FGameplayTag SlotTag) const
{
    // Simple membership check against the designer-configured SupportedSlots array.
    return SupportedSlots.Contains(SlotTag);
}

// --------------------------------------------------------------------
// Private: Startup / inventory linkage
// --------------------------------------------------------------------

void URiftEquipmentComponent::OnPawnReady()
{
    // Ensure our owner is actually a pawn (this component should only live on APawn).
    APawn* OwningPawn = GetOwner<APawn>();
    if (!OwningPawn)
    {
        return;
    }

    // Resolve the inventory via IRiftInventoryInterface.
    // Check the pawn first — supports configurations where the inventory lives directly
    // on the pawn rather than on the PlayerState.
    URiftInventoryComponent* Inv = nullptr;

    if (OwningPawn->Implements<URiftInventoryInterface>())
    {
        Inv = Cast<URiftInventoryComponent>(
            IRiftInventoryInterface::Execute_GetInventoryComponent(OwningPawn));
    }

    if (!Inv)
    {
        // Fall back to the PlayerState. On a listen-server or in PIE, BeginPlay can fire
        // before the pawn is possessed, so PlayerState may not be set yet — retry if so.
        APlayerState* PS = OwningPawn->GetPlayerState();
        if (!PS)
        {
            GetWorld()->GetTimerManager().SetTimerForNextTick(this, &URiftEquipmentComponent::OnPawnReady);
            return;
        }

        if (PS->Implements<URiftInventoryInterface>())
        {
            Inv = Cast<URiftInventoryComponent>(
                IRiftInventoryInterface::Execute_GetInventoryComponent(PS));
        }

        // If IRiftInventoryInterface is not implemented, fall back to component search.
        // This makes IRiftInventoryInterface optional — any pawn or PlayerState that
        // simply has a URiftInventoryComponent will work without the interface boilerplate.
        if (!Inv)
        {
            Inv = PS->FindComponentByClass<URiftInventoryComponent>();
        }
    }

    if (!Inv)
    {
        // Last-resort fallback: check the pawn itself by component search.
        Inv = OwningPawn->FindComponentByClass<URiftInventoryComponent>();
    }

    if (!Inv)
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftEquipmentComponent: Could not find URiftInventoryComponent on %s or its PlayerState. Equipment will not function."),
            *GetOwner()->GetName());
        return;
    }

    // Store a weak reference so we do not prevent the inventory owner from being GC'd.
    LinkedInventory = Inv;

    // WaitForInitialized fires the delegate once the inventory has finished async setup
    // (loading item data assets, building containers, etc.).
    // This prevents us from trying to move items into containers that do not yet exist.
    Inv->WaitForInitialized(FOnRiftInventoryInitializedDelegate::CreateUObject(
        this, &URiftEquipmentComponent::OnInventoryReady));
}

void URiftEquipmentComponent::OnInventoryReady(bool bSuccess)
{
    UE_LOG(LogTemp, Warning, TEXT("URiftEquipmentComponent::OnInventoryReady — bSuccess: %s"), bSuccess ? TEXT("true") : TEXT("false"));

    if (!bSuccess || !LinkedInventory.IsValid())
    {
        return;
    }

    // The operations below are server-only. The owning client runs OnPawnReady solely
    // to populate LinkedInventory for TryResolveEquipmentSlotItem; it does not need
    // event bindings, ability grants, or startup scans — those are server concerns.
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    // React to forcible item removals (admin strip, item expiry, etc.).
    LinkedInventory->OnItemEvent.AddDynamic(this, &URiftEquipmentComponent::OnInventoryItemEvent);

    // React to items being dragged directly into or out of slot containers via the UI,
    // bypassing the explicit EquipItem / UnequipItem API.
    LinkedInventory->OnItemMoved.AddDynamic(this, &URiftEquipmentComponent::OnInventoryItemMoved);
    UE_LOG(LogTemp, Warning, TEXT("URiftEquipmentComponent::OnInventoryReady — Bound to OnItemMoved. SupportedSlots: %d"), SupportedSlots.Num());

    // Auto-grant the equip/unequip abilities so Blueprint and C++ callers can send
    // gameplay events without needing a startup ability set asset on every pawn.
    // Skipped if bAutoGrantEquipAbilities is false (manual ability management opt-out).
    if (bAutoGrantEquipAbilities)
    {
        if (UAbilitySystemComponent* ASC = GetASC())
        {
            FGameplayAbilitySpec EquipSpec(URiftAbility_Equip::StaticClass(), 1, INDEX_NONE, this);
            if (!ASC->FindAbilitySpecFromClass(URiftAbility_Equip::StaticClass()))
            {
                ASC->GiveAbility(EquipSpec);
            }

            FGameplayAbilitySpec UnequipSpec(URiftAbility_Unequip::StaticClass(), 1, INDEX_NONE, this);
            if (!ASC->FindAbilitySpecFromClass(URiftAbility_Unequip::StaticClass()))
            {
                ASC->GiveAbility(UnequipSpec);
            }

            FGameplayAbilitySpec DropSpec(URiftAbility_Drop::StaticClass(), 1, INDEX_NONE, this);
            if (!ASC->FindAbilitySpecFromClass(URiftAbility_Drop::StaticClass()))
            {
                ASC->GiveAbility(DropSpec);
            }
        }
    }

    // Startup scan — re-register items already sitting in the equipment container.
    // This handles the persistence load case: the inventory is populated from a save file
    // before BeginPlay runs on the equipment component, so items may already be equipped
    // without being tracked in EquipmentSlots yet. Iterates the equipment container and
    // calls FinishEquip for each item whose slot tag is supported and not yet occupied.
    URiftContainer* EquipmentContainer = LinkedInventory->GetContainerByTag(Tag_Rift_Container_Equipment);
    if (IsValid(EquipmentContainer))
    {
        // Populate per-slot tag requirements so MoveItemToContainerAtSlot can reject
        // items dropped at the wrong slot (e.g. a chest piece on the Primary Hand slot).
        // SupportedSlots[i] defines the expected equipment slot tag at container slot i.
        EquipmentContainer->SlotTagRequirements = SupportedSlots;
        UE_LOG(LogTemp, Warning, TEXT("URiftEquipmentComponent — SlotTagRequirements populated: %d entries"), EquipmentContainer->SlotTagRequirements.Num());
        for (int32 i = 0; i < EquipmentContainer->SlotTagRequirements.Num(); ++i)
        {
            UE_LOG(LogTemp, Warning, TEXT("  Slot %d -> %s"), i, *EquipmentContainer->SlotTagRequirements[i].ToString());
        }

        for (URiftItemInstance* Item : EquipmentContainer->GetAllItems())
        {
            if (!IsValid(Item))
            {
                continue;
            }

            const URiftFragment_Equippable* Fragment = Item->FindFragment<URiftFragment_Equippable>();
            if (!Fragment)
            {
                continue;
            }

            const FGameplayTag SlotTag = Fragment->GetEquipmentSlotTag();
            if (IsSlotSupported(SlotTag) && !IsSlotOccupied(SlotTag))
            {
                FinishEquip(Item, SlotTag);
            }
        }
    }
}

void URiftEquipmentComponent::OnInventoryItemEvent(URiftItemInstance* Item, FGameplayTag EventTag)
{
    // We only care about forcible item removal. Equipped and Unequipped events are already
    // handled by EquipItem / UnequipItem; listening for them here would cause double-processing.
    if (EventTag == Tag_Rift_Event_Item_Removed)
    {
        for (int32 i = EquipmentSlots.Entries.Num() - 1; i >= 0; --i)
        {
            if (EquipmentSlots.Entries[i].Item == Item)
            {
                FinishUnequip(Item, EquipmentSlots.Entries[i].SlotTag);
                break;
            }
        }
    }
}

void URiftEquipmentComponent::OnInventoryItemMoved(URiftItemInstance* Item,
    URiftContainer* FromContainer, URiftContainer* ToContainer)
{
    if (!IsValid(Item))
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("URiftEquipmentComponent::OnInventoryItemMoved — Item: %s, From: %s, To: %s"),
        *GetNameSafe(Item),
        IsValid(FromContainer) ? *FromContainer->GetContainerTag().ToString() : TEXT("NULL"),
        IsValid(ToContainer)   ? *ToContainer->GetContainerTag().ToString()   : TEXT("NULL"));

    // --- Equip path: item moved INTO the equipment container or a per-slot container ---
    // Supports two layouts:
    //   1. Single shared container tagged Tag_Rift_Container_Equipment (slot determined by fragment)
    //   2. Per-slot containers tagged with their slot tag (e.g. Rift.Slot.Weapon.Primary)
    if (IsValid(ToContainer))
    {
        const FGameplayTag ToTag = ToContainer->GetContainerTag();
        const bool bToMainContainer  = (ToTag == Tag_Rift_Container_Equipment);
        const bool bToPerSlotContainer = !bToMainContainer && IsSlotSupported(ToTag);

        if (bToMainContainer || bToPerSlotContainer)
        {
            const URiftFragment_Equippable* Fragment = Item->FindFragment<URiftFragment_Equippable>();
            if (Fragment)
            {
                const FGameplayTag SlotTag = Fragment->GetEquipmentSlotTag();

                // For per-slot containers the container tag IS the slot tag — validate they match.
                if (bToPerSlotContainer && SlotTag != ToTag)
                {
                    return;
                }

                if (IsSlotSupported(SlotTag))
                {
                    // Swap case: slot is occupied by a different item — unequip it first
                    // before equipping the new one. This handles the scenario where
                    // MoveItemToContainerAtSlot broadcasts the incoming item's OnItemMoved
                    // before the displaced item's, so the slot still appears occupied.
                    if (IsSlotOccupied(SlotTag))
                    {
                        URiftItemInstance* OldItem = GetItemInSlot(SlotTag);
                        if (IsValid(OldItem) && OldItem != Item)
                        {
                            FinishUnequip(OldItem, SlotTag);
                        }
                    }

                    if (!IsSlotOccupied(SlotTag))
                    {
                        FinishEquip(Item, SlotTag);
                    }
                    return;
                }
            }
        }
    }

    // --- Unequip path: item moved OUT OF the equipment container or a per-slot container ---
    // In a swap, the old item was already unequipped in the equip path above and the
    // new item is now registered in the slot. Guard against firing a second unequip
    // for the displaced item by confirming it is still the one tracked in the slot.
    if (IsValid(FromContainer))
    {
        const FGameplayTag FromTag = FromContainer->GetContainerTag();
        const bool bFromMainContainer    = (FromTag == Tag_Rift_Container_Equipment);
        const bool bFromPerSlotContainer = !bFromMainContainer && IsSlotSupported(FromTag);

        if (bFromMainContainer || bFromPerSlotContainer)
        {
            const URiftFragment_Equippable* Fragment = Item->FindFragment<URiftFragment_Equippable>();
            if (Fragment)
            {
                const FGameplayTag SlotTag = Fragment->GetEquipmentSlotTag();
                if (IsSlotSupported(SlotTag) && IsSlotOccupied(SlotTag) && GetItemInSlot(SlotTag) == Item)
                {
                    FinishUnequip(Item, SlotTag);
                }
            }
        }
    }
}

// --------------------------------------------------------------------
// Private: Equip / unequip helpers (no item move)
// --------------------------------------------------------------------

void URiftEquipmentComponent::FinishEquip(URiftItemInstance* Item, FGameplayTag SlotTag)
{
    // Record the slot entry and mark dirty for replication.
    // ItemDefinition is a content asset available on ALL clients, so it lets
    // URiftMutableEquipmentComponent work on simulated proxies where Item is null.
    FRiftEquipmentSlotEntry NewEntry(SlotTag, Item);
    NewEntry.ItemDefinition = IsValid(Item) ? Item->GetDefinition() : nullptr;
    EquipmentSlots.Entries.Add(NewEntry);
    EquipmentSlots.MarkItemDirty(EquipmentSlots.Entries.Last());

    // Notify inventory listeners (UI, quest system, etc.) that an item was equipped.
    if (LinkedInventory.IsValid())
    {
        LinkedInventory->BroadcastItemEvent(Item, Tag_Rift_Event_Item_Equipped);
    }

    // Grant GAS ability sets from URiftFragment_Equippable_GAS (no-op for base fragment).
    GrantItemAbilities(Item, SlotTag);

    // Weapon-specific: spawn actors and grant weapon state tag.
    const URiftFragment_Equippable_Weapon* WeaponFragment = Item->FindFragment<URiftFragment_Equippable_Weapon>();
    if (WeaponFragment)
    {
        // Spawn primary and/or off-hand weapon actors if sockets are defined.
        if (!WeaponFragment->GetPrimarySocketName().IsNone() || !WeaponFragment->GetOffHandSocketName().IsNone())
        {
            SpawnWeaponActors(Item, SlotTag, WeaponFragment);
        }

        // Push the weapon state tag to the ASC so Mover and ABPs can react.
        if (WeaponFragment->GetWeaponStateTag().IsValid())
        {
            if (UAbilitySystemComponent* ASC = GetASC())
            {
                ASC->AddLooseGameplayTag(WeaponFragment->GetWeaponStateTag());
            }
        }
    }

    // Broadcast server-side delegate. Clients receive theirs via PostReplicatedAdd.
    OnItemEquipped.Broadcast(Item, SlotTag, IsValid(Item) ? Item->GetDefinition() : nullptr);
}

void URiftEquipmentComponent::FinishUnequip(URiftItemInstance* Item, FGameplayTag SlotTag)
{
    // Revoke GAS grants for this slot.
    RevokeItemAbilities(SlotTag);

    // Weapon-specific: destroy actors and remove weapon state tag.
    const URiftFragment_Equippable_Weapon* WeaponFragment = Item->FindFragment<URiftFragment_Equippable_Weapon>();
    if (WeaponFragment)
    {
        DestroyWeaponActors(SlotTag);

        if (WeaponFragment->GetWeaponStateTag().IsValid())
        {
            if (UAbilitySystemComponent* ASC = GetASC())
            {
                ASC->RemoveLooseGameplayTag(WeaponFragment->GetWeaponStateTag());
            }
        }
    }

    // Notify inventory listeners before removing the entry.
    if (LinkedInventory.IsValid())
    {
        LinkedInventory->BroadcastItemEvent(Item, Tag_Rift_Event_Item_Unequipped);
    }

    // Broadcast server-side delegate. Clients receive theirs via PreReplicatedRemove.
    OnItemUnequipped.Broadcast(Item, SlotTag, IsValid(Item) ? Item->GetDefinition() : nullptr);

    // Remove the slot entry and mark dirty for replication.
    EquipmentSlots.Entries.RemoveAll([&SlotTag](const FRiftEquipmentSlotEntry& E)
    {
        return E.SlotTag == SlotTag;
    });
    EquipmentSlots.MarkArrayDirty();
}

// --------------------------------------------------------------------
// Private: GAS ability grant / revoke
// --------------------------------------------------------------------

void URiftEquipmentComponent::GrantItemAbilities(URiftItemInstance* Item, FGameplayTag SlotTag)
{
    // Nothing to grant if the pawn has no ASC or the item reference is bad.
    UAbilitySystemComponent* ASC = GetASC();
    if (!ASC || !Item)
    {
        return;
    }

    // Only URiftFragment_Equippable_GAS carries ability sets. Items using the base
    // URiftFragment_Equippable (non-GAS) skip this step entirely.
    const URiftFragment_Equippable_GAS* GASFragment = Item->FindFragment<URiftFragment_Equippable_GAS>();
    if (!GASFragment || GASFragment->GetAbilitySets().IsEmpty())
    {
        return;
    }

    // Accumulate all grants from every ability set into a single handle struct keyed by slot.
    // This lets RevokeItemAbilities undo every grant for the slot with a single RemoveFromASC call.
    FRiftAbilitySetGrantedHandles& Handles = GrantedAbilitySetHandles.FindOrAdd(SlotTag);

    for (const TObjectPtr<URiftAbilitySet>& AbilitySet : GASFragment->GetAbilitySets())
    {
        if (IsValid(AbilitySet))
        {
            AbilitySet->GrantToASC(ASC, Item, Handles);
        }
    }
}

void URiftEquipmentComponent::RevokeItemAbilities(FGameplayTag SlotTag)
{
    UAbilitySystemComponent* ASC = GetASC();
    if (!ASC)
    {
        return;
    }

    // Find returns nullptr if no ability sets were granted for this slot
    // (e.g. items using base URiftFragment_Equippable with no GAS fragment).
    if (FRiftAbilitySetGrantedHandles* Handles = GrantedAbilitySetHandles.Find(SlotTag))
    {
        // RemoveFromASC revokes abilities, removes effects, and unregisters attribute sets
        // in one atomic call, then clears the handle arrays inside the struct.
        Handles->RemoveFromASC(ASC);

        // Remove the map entry so stale structs do not accumulate over equip cycles.
        GrantedAbilitySetHandles.Remove(SlotTag);
    }
}

// --------------------------------------------------------------------
// Private: Weapon actor spawn / destroy
// --------------------------------------------------------------------

void URiftEquipmentComponent::SpawnWeaponActors(URiftItemInstance* Item, FGameplayTag SlotTag, const URiftFragment_Equippable_Weapon* Fragment)
{
    // Prefer the per-item class from the fragment, fall back to the per-pawn class on the
    // component, then fall back to the base ARiftWeaponActor. The fragment stores AActor
    // (to avoid a circular module dependency) so we validate it is actually a weapon actor.
    TSubclassOf<ARiftWeaponActor> ActorClass;
    if (UClass* FragmentClass = Fragment->GetWeaponActorClass().Get())
    {
        if (FragmentClass->IsChildOf<ARiftWeaponActor>())
        {
            ActorClass = FragmentClass;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("URiftEquipmentComponent: WeaponActorClass on fragment is not a subclass of ARiftWeaponActor — ignoring."));
        }
    }
    if (!ActorClass)
    {
        ActorClass = WeaponActorClass ? WeaponActorClass : TSubclassOf<ARiftWeaponActor>(ARiftWeaponActor::StaticClass());
    }

    USkeletalMeshComponent* BodyMesh = FindBodyMeshComponent();
    if (!BodyMesh)
    {
        UE_LOG(LogTemp, Warning, TEXT("URiftEquipmentComponent: No USkeletalMeshComponent tagged Rift.Component.BodyMesh found on %s — weapon actors not spawned."),
            *GetOwner()->GetName());
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner                          = GetOwner();
    SpawnParams.Instigator                     = GetOwner<APawn>();
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // --- Primary hand ---
    if (!Fragment->GetPrimarySocketName().IsNone())
    {
        ARiftWeaponActor* PrimaryActor = GetWorld()->SpawnActor<ARiftWeaponActor>(ActorClass, SpawnParams);
        if (PrimaryActor)
        {
            PrimaryActor->AttachToComponent(BodyMesh,
                FAttachmentTransformRules::SnapToTargetIncludingScale,
                Fragment->GetPrimarySocketName());
            PrimaryActor->SetupForItem(Item);
            SpawnedWeaponActors.Add(SlotTag, PrimaryActor);
        }
    }

    // --- Off-hand (shield or two-handed grip) ---
    if (!Fragment->GetOffHandSocketName().IsNone())
    {
        ARiftWeaponActor* OffHandActor = GetWorld()->SpawnActor<ARiftWeaponActor>(ActorClass, SpawnParams);
        if (OffHandActor)
        {
            OffHandActor->AttachToComponent(BodyMesh,
                FAttachmentTransformRules::SnapToTargetIncludingScale,
                Fragment->GetOffHandSocketName());
            OffHandActor->SetupForItem(Item);
            SpawnedOffHandActors.Add(SlotTag, OffHandActor);
        }
    }
}

void URiftEquipmentComponent::DestroyWeaponActors(FGameplayTag SlotTag)
{
    if (TObjectPtr<ARiftWeaponActor>* ActorPtr = SpawnedWeaponActors.Find(SlotTag))
    {
        if (*ActorPtr) { (*ActorPtr)->Destroy(); }
        SpawnedWeaponActors.Remove(SlotTag);
    }

    if (TObjectPtr<ARiftWeaponActor>* ActorPtr = SpawnedOffHandActors.Find(SlotTag))
    {
        if (*ActorPtr) { (*ActorPtr)->Destroy(); }
        SpawnedOffHandActors.Remove(SlotTag);
    }
}

// --------------------------------------------------------------------
// Private: Helpers
// --------------------------------------------------------------------

void URiftEquipmentComponent::TryResolveEquipmentSlotItem(FGameplayTag SlotTag)
{
    // Server already holds the live item pointer set by FinishEquip — no resolution needed.
    // Non-owning clients have no inventory replication so there is nothing to resolve.
    // Only the owning client needs this path.
    APawn* OwningPawn = GetOwner<APawn>();
    if (!OwningPawn || !OwningPawn->IsLocallyControlled() || OwningPawn->HasAuthority())
    {
        return;
    }

    FRiftEquipmentSlotEntry* Entry = EquipmentSlots.FindEntryForSlot(SlotTag);
    if (!Entry || Entry->Item)
    {
        return;  // Slot gone or already resolved.
    }

    if (!LinkedInventory.IsValid())
    {
        // LinkedInventory not yet set — OnPawnReady is still pending. Retry next tick.
        GetWorld()->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateWeakLambda(this, [this, SlotTag]()
            {
                TryResolveEquipmentSlotItem(SlotTag);
            }));
        return;
    }

    URiftContainer* SlotContainer = LinkedInventory->GetContainerByTag(SlotTag);
    if (!IsValid(SlotContainer))
    {
        // Container not yet replicated (different actor channel — no ordering guarantee).
        // Retry next tick; the container should arrive shortly.
        GetWorld()->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateWeakLambda(this, [this, SlotTag]()
            {
                TryResolveEquipmentSlotItem(SlotTag);
            }));
        return;
    }

    TArray<URiftItemInstance*> Items = SlotContainer->GetAllItems();
    if (Items.Num() > 0)
    {
        Entry->Item = Items[0];
    }
    else
    {
        // Container replicated but item reconstruction not yet complete (FRiftSlotList
        // PostReplicatedAdd may be pending on the same tick). Retry once next tick.
        GetWorld()->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateWeakLambda(this, [this, SlotTag]()
            {
                TryResolveEquipmentSlotItem(SlotTag);
            }));
    }
}

UAbilitySystemComponent* URiftEquipmentComponent::GetASC() const
{
    // Check the pawn first (ASC-on-pawn setup).
    if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
    {
        if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
        {
            return ASC;
        }
    }

    // Fall back to the PlayerState (Lyra-style ASC-on-PlayerState setup).
    if (const APawn* Pawn = GetOwner<APawn>())
    {
        if (APlayerState* PS = Pawn->GetPlayerState())
        {
            if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
            {
                return ASI->GetAbilitySystemComponent();
            }
        }
    }

    return nullptr;
}

URiftItemDefinition* URiftEquipmentComponent::GetDefinitionForSlot(FGameplayTag SlotTag) const
{
    const FRiftEquipmentSlotEntry* Entry = EquipmentSlots.FindEntryForSlot(SlotTag);
    return Entry ? Entry->ItemDefinition.Get() : nullptr;
}

USkeletalMeshComponent* URiftEquipmentComponent::FindBodyMeshComponent() const
{
    APawn* OwningPawn = GetOwner<APawn>();
    if (!OwningPawn)
    {
        return nullptr;
    }

    const FName BodyMeshTagName = Tag_Rift_Component_BodyMesh.GetTag().GetTagName();
    TArray<USkeletalMeshComponent*> SkelComps;
    OwningPawn->GetComponents<USkeletalMeshComponent>(SkelComps);

    for (USkeletalMeshComponent* Comp : SkelComps)
    {
        if (Comp->ComponentTags.Contains(BodyMeshTagName))
        {
            return Comp;
        }
    }

    return nullptr;
}

// ====================================================================
// FastArray replication callbacks for FRiftEquipmentSlotEntry
//
// These are defined here — not in a FRiftEquipmentSlotState.cpp — because
// they need to call URiftEquipmentComponent::OnItemEquipped / OnItemUnequipped.
// Including URiftEquipmentComponent.h from FRiftEquipmentSlotState.cpp would
// create a circular include (SlotState.h is included by EquipmentComponent.h).
// Placing the implementations in this .cpp file gives us access to the full
// URiftEquipmentComponent definition with no circular dependency.
// ====================================================================

void FRiftEquipmentSlotEntry::PreReplicatedRemove(const FRiftEquipmentSlotList& InArraySerializer)
{
    UE_LOG(LogTemp, Warning, TEXT("FRiftEquipmentSlotEntry::PreReplicatedRemove — Slot: %s, Item: %s, Def: %s"),
        *SlotTag.ToString(),
        Item ? *Item->GetName() : TEXT("null"),
        ItemDefinition ? *ItemDefinition->GetName() : TEXT("null"));

    if (InArraySerializer.OwnerComponent && (Item || ItemDefinition))
    {
        InArraySerializer.OwnerComponent->OnItemUnequipped.Broadcast(Item, SlotTag, ItemDefinition.Get());
    }
}

void FRiftEquipmentSlotEntry::PostReplicatedAdd(const FRiftEquipmentSlotList& InArraySerializer)
{
    UE_LOG(LogTemp, Warning, TEXT("FRiftEquipmentSlotEntry::PostReplicatedAdd — Slot: %s, Item: %s, Def: %s"),
        *SlotTag.ToString(),
        Item ? *Item->GetName() : TEXT("null"),
        ItemDefinition ? *ItemDefinition->GetName() : TEXT("null"));

    if (!InArraySerializer.OwnerComponent || !ItemDefinition)
    {
        return;
    }

    // Item is NotReplicated — broadcast immediately with whatever we have (Item may be null).
    // ItemDefinition is always valid here and is sufficient for Mutable visual setup on all clients.
    InArraySerializer.OwnerComponent->OnItemEquipped.Broadcast(Item, SlotTag, ItemDefinition.Get());

    // Attempt to resolve the Item pointer from the inventory container. On the owning client,
    // the inventory container may already hold the reconstructed item instance if its
    // PostReplicatedAdd ran in the same replication batch. If not, the call defers one tick.
    // Non-owning clients have no inventory replication and skip the resolve (Item stays null).
    InArraySerializer.OwnerComponent->TryResolveEquipmentSlotItem(SlotTag);
}

void FRiftEquipmentSlotEntry::PostReplicatedChange(const FRiftEquipmentSlotList& InArraySerializer)
{
    // Fires when a replicated field (ItemDefinition) changes for an existing entry —
    // e.g., a slot swap where a different item now occupies this slot.
    // Item is NotReplicated so this never fires due to Item changing.
    if (!InArraySerializer.OwnerComponent || !ItemDefinition)
    {
        return;
    }

    InArraySerializer.OwnerComponent->OnItemEquipped.Broadcast(Item, SlotTag, ItemDefinition.Get());

    // Re-attempt resolution in case the definition changed due to a swap.
    if (!Item)
    {
        InArraySerializer.OwnerComponent->TryResolveEquipmentSlotItem(SlotTag);
    }
}

// --------------------------------------------------------------------
// FRiftEquipmentSlotList helpers
// --------------------------------------------------------------------

FRiftEquipmentSlotEntry* FRiftEquipmentSlotList::FindEntryForSlot(FGameplayTag SlotTag)
{
    // Linear search is intentional. Equipment arrays are tiny (2–8 entries at most),
    // so a hash map would add allocation overhead for no measurable benefit.
    return Entries.FindByPredicate([&SlotTag](const FRiftEquipmentSlotEntry& E)
    {
        return E.SlotTag == SlotTag;
    });
}

const FRiftEquipmentSlotEntry* FRiftEquipmentSlotList::FindEntryForSlot(FGameplayTag SlotTag) const
{
    // Const overload for read-only call sites (query functions, const component methods, etc.).
    return Entries.FindByPredicate([&SlotTag](const FRiftEquipmentSlotEntry& E)
    {
        return E.SlotTag == SlotTag;
    });
}
