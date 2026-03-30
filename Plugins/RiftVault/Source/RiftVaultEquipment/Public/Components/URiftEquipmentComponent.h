#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Data/FRiftEquipmentSlotState.h"
#include "Data/URiftAbilitySet.h"
#include "URiftEquipmentComponent.generated.h"

class URiftInventoryComponent;
class URiftContainer;
class URiftItemInstance;
class URiftItemDefinition;
class URiftFragment_Equippable;
class URiftFragment_Equippable_Weapon;
class ARiftWeaponActor;
class UAbilitySystemComponent;
class USkeletalMeshComponent;

/**
 * Broadcast on the server immediately after EquipItem() succeeds.
 * Also broadcast on clients via the FastArray PostReplicatedAdd callback when replication arrives.
 *
 * @param Item     The item instance that was equipped.
 * @param SlotTag  The slot it was placed into.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRiftItemEquipped,   URiftItemInstance*, Item, FGameplayTag, SlotTag, URiftItemDefinition*, ItemDefinition);

/**
 * Broadcast on the server immediately after UnequipItem() succeeds.
 * Also broadcast on clients via the FastArray PreReplicatedRemove callback when replication arrives.
 *
 * @param Item            The item instance that was unequipped.
 * @param SlotTag         The slot it was removed from.
 * @param ItemDefinition  The item's definition asset. Always valid — passed directly from the
 *                        slot entry so observers never need to look it up from the live array
 *                        (which can be in mid-swap when FastArray fires additions before removals).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRiftItemUnequipped, URiftItemInstance*, Item, FGameplayTag, SlotTag, URiftItemDefinition*, ItemDefinition);

/**
 * Manages a pawn's equipment slots — what item occupies each slot, GAS ability grants,
 * weapon actor lifetime, and delegation to Mutable visual components.
 *
 * Placement:
 *   Add this component to APawn (not ACharacter — the design intentionally avoids
 *   ACharacter to remain compatible with the Mover plugin and non-humanoid pawns).
 *   The companion URiftMutableEquipmentComponent can be added alongside it when
 *   Mutable body-armor visuals are needed.
 *
 * Inventory relationship:
 *   Item ownership lives on URiftInventoryComponent on APlayerState.
 *   This component holds weak references to items and borrows access to the inventory
 *   only to move items between containers (backpack ↔ equipment slot container) and
 *   to broadcast item events (Equipped / Unequipped).
 *
 * Replication model:
 *   - EquipmentSlots (FRiftEquipmentSlotList) is replicated via Fast TArray.
 *   - Server writes the authoritative slot state; clients receive diffs.
 *   - FastArray callbacks on FRiftEquipmentSlotEntry fire OnItemEquipped /
 *     OnItemUnequipped on clients so delegates stay in sync without extra RPCs.
 *   - GAS ability set handles (GrantedAbilitySetHandles) and weapon actors (SpawnedWeaponActors)
 *     are server-only; clients rely on ability replication and actor replication respectively.
 *
 * Full activation flow:
 *   1. Client UI sends gameplay event Tag_Rift_Ability_Equip to the owning pawn.
 *   2. URiftAbility_Equip activates (ServerOnly) and calls EquipItem(Item, SlotTag).
 *   3. EquipItem validates the request, records the slot, moves the item in the
 *      inventory, grants GAS abilities, spawns a weapon actor if the fragment
 *      specifies a socket, and fires OnItemEquipped on the server.
 *   4. FRiftEquipmentSlotList replicates the new entry to clients, triggering
 *      PostReplicatedAdd which fires OnItemEquipped on each client.
 *   5. URiftMutableEquipmentComponent (bound to OnItemEquipped) applies body-mesh
 *      Mutable parameters on the server; Mutable replication handles client visuals.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup=(RiftVault), meta=(BlueprintSpawnableComponent))
class RIFTVAULTEQUIPMENT_API URiftEquipmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    /**
     * Fired on the server immediately after EquipItem() records a new slot entry.
     * Fired on clients by FRiftEquipmentSlotEntry::PostReplicatedAdd when replication arrives.
     * Bind here to react to equip events on both server and all clients (e.g. UI, VFX, audio).
     */
    UPROPERTY(BlueprintAssignable, Category = "RiftVault|Equipment|Delegates")
    FOnRiftItemEquipped OnItemEquipped;

    /**
     * Fired on the server immediately before the slot entry is removed in UnequipItem().
     * Fired on clients by FRiftEquipmentSlotEntry::PreReplicatedRemove when replication arrives.
     * Bind here to react to unequip events on both server and all clients.
     */
    UPROPERTY(BlueprintAssignable, Category = "RiftVault|Equipment|Delegates")
    FOnRiftItemUnequipped OnItemUnequipped;

    /** Constructor. Enables replication and sets the EquipmentSlots back-pointer. */
    URiftEquipmentComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    /** On server: resolves the linked inventory via the timer-retry pattern. */
    virtual void BeginPlay() override;

    /** On server: destroys all spawned weapon actors and clears ability handle bookkeeping. */
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /**
     * Registers EquipmentSlots for replication.
     * Called by the Unreal network system to build the replication property list.
     */
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /**
     * Called per-channel to replicate sub-objects owned by this component.
     * Item instances are owned and replicated by URiftInventoryComponent, so we
     * do not replicate any sub-objects here — slot entries carry pointers only.
     */
    virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

    // ------------------------------------------------------------------
    // Equipment API  (server-authoritative — BlueprintAuthorityOnly)
    // ------------------------------------------------------------------

    /**
     * Equips Item into SlotTag on the server.
     *
     * Validation checklist (returns false + logs a warning if any fails):
     *   1. Item and SlotTag are non-null / valid.
     *   2. SlotTag is listed in SupportedSlots.
     *   3. The slot is not already occupied.
     *   4. Item has a URiftFragment_Equippable.
     *   5. The fragment's slot tag matches SlotTag exactly.
     *
     * On success:
     *   - Adds an FRiftEquipmentSlotEntry to EquipmentSlots (marks it dirty for replication).
     *   - Moves the item into its per-slot URiftContainer in the linked inventory.
     *   - Broadcasts Tag_Rift_Event_Item_Equipped on the inventory.
     *   - Grants GAS abilities listed in the fragment.
     *   - Spawns and attaches an ARiftWeaponActor if the fragment has a WeaponSocketName.
     *   - Fires OnItemEquipped.
     *
     * @param Item     The item instance to equip. Must belong to the linked inventory.
     * @param SlotTag  The target equipment slot tag (must be in SupportedSlots).
     * @return true if equip succeeded.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Equipment")
    bool EquipItem(URiftItemInstance* Item, FGameplayTag SlotTag);

    /**
     * Unequips the item currently in SlotTag on the server.
     *
     * On success:
     *   - Revokes all GAS abilities that were granted when the item was equipped.
     *   - Destroys the spawned ARiftWeaponActor for this slot (if any).
     *   - Broadcasts Tag_Rift_Event_Item_Unequipped on the inventory.
     *   - Moves the item back to the backpack container.
     *   - Fires OnItemUnequipped.
     *   - Removes the FRiftEquipmentSlotEntry and marks the array dirty for replication.
     *
     * @param SlotTag  The slot to unequip.
     * @return true if a slot was occupied and successfully cleared.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Equipment")
    bool UnequipItem(FGameplayTag SlotTag);

    // ------------------------------------------------------------------
    // Query API  (safe to call from server or clients)
    // ------------------------------------------------------------------

    /**
     * Returns the item instance in the given slot, or nullptr if the slot is empty.
     * Works on both server and clients — reads from the replicated EquipmentSlots array.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Equipment")
    URiftItemInstance* GetItemInSlot(FGameplayTag SlotTag) const;

    /**
     * Convenience wrapper around GetItemInSlot — returns true if the slot holds an item.
     * Safe on both server and clients.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Equipment")
    bool IsSlotOccupied(FGameplayTag SlotTag) const;

    /**
     * Collects and returns all non-null item instances from every occupied slot.
     * Reads from the replicated array, so results are valid on server and clients.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Equipment")
    TArray<URiftItemInstance*> GetAllEquippedItems() const;

    /**
     * Returns true if SlotTag is listed in SupportedSlots.
     * Used by EquipItem to reject attempts to use unsupported slots on this pawn.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Equipment")
    bool IsSlotSupported(FGameplayTag SlotTag) const;

    /**
     * Returns the inventory component this equipment component is linked to.
     * Null until OnPawnReady resolves the PlayerState's URiftInventoryComponent.
     * Server-side link; not replicated to clients.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Equipment")
    URiftInventoryComponent* GetLinkedInventory() const { return LinkedInventory.Get(); }

    /**
     * Returns the immutable list of slot tags this pawn supports.
     * Configured per-pawn in the Blueprint defaults via the SupportedSlots property.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Equipment")
    const TArray<FGameplayTag>& GetSupportedSlots() const { return SupportedSlots; }

    /**
     * Returns the item definition for the given slot, or nullptr if the slot is empty.
     * Works on ALL clients (unlike GetItemInSlot, which returns null for non-owning clients).
     * Used by URiftMutableEquipmentComponent on simulated proxies to apply Mutable parameters.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Equipment")
    URiftItemDefinition* GetDefinitionForSlot(FGameplayTag SlotTag) const;

    /**
     * Attempts to resolve the Item pointer for the given slot entry from the linked
     * inventory's slot container. Called on the owning client from PostReplicatedAdd
     * after the OnItemEquipped broadcast, because Item is NotReplicated and must be
     * populated via a local container lookup.
     *
     * If the inventory container is not yet replicated (different actor channel, no
     * ordering guarantee), the call defers one tick and retries automatically.
     *
     * Public so FRiftEquipmentSlotEntry's FastArray callbacks (which are a separate struct)
     * can call it without needing friend access.
     *
     * @param SlotTag  The equipment slot whose entry needs its Item pointer resolved.
     */
    void TryResolveEquipmentSlotItem(FGameplayTag SlotTag);

protected:

    /**
     * The set of equipment slot tags this pawn supports.
     * Set this in the pawn's Blueprint defaults (e.g. Rift.Slot.Weapon.Primary, Rift.Slot.Armor.Chest).
     * Only slots listed here can be occupied. EquipItem will reject any slot not in this list.
     * Uses the meta-filter "Rift.Slot" to restrict the tag picker in the editor.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment",
        meta = (Categories = "Rift.Slot"))
    TArray<FGameplayTag> SupportedSlots;

    /**
     * If true (default), URiftAbility_Equip and URiftAbility_Unequip are automatically
     * granted to the pawn's ASC once the inventory is ready. This eliminates the need
     * to add these abilities to a startup ability set on the pawn.
     * Set to false if you manage ability grants manually via your own ability set assets.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
    bool bAutoGrantEquipAbilities = true;

    /**
     * If true (default), items picked up or added to the inventory are automatically
     * equipped when their target equipment slot is empty.
     * Only applies to items with URiftFragment_Equippable whose slot is supported
     * and currently unoccupied.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
    bool bAutoEquipOnPickup = true;

    /**
     * The weapon actor class to spawn when equipping an item that has a WeaponSocketName.
     * Defaults to ARiftWeaponActor (the base class) if left unset.
     * Override per-pawn Blueprint to use a subclass that has custom collision, audio, or VFX.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
    TSubclassOf<ARiftWeaponActor> WeaponActorClass;

private:

    /**
     * The replicated array of active slot entries.
     * Uses Fast TArray delta serialization to minimize bandwidth — only changed
     * entries are sent to clients on each replication update.
     */
    UPROPERTY(Replicated)
    FRiftEquipmentSlotList EquipmentSlots;

    /**
     * Weak reference to the URiftInventoryComponent on the owning pawn's PlayerState.
     * Resolved in OnPawnReady() after possession. Not replicated — server only.
     * TWeakObjectPtr prevents a hard reference cycle between pawn and player state.
     */
    UPROPERTY()
    TWeakObjectPtr<URiftInventoryComponent> LinkedInventory;

    /**
     * Stores all GAS grants made when an item was equipped, keyed by slot tag.
     * Each entry is an accumulated FRiftAbilitySetGrantedHandles from all ability sets
     * on the equipped item's URiftFragment_Equippable_GAS. RevokeItemAbilities calls
     * RemoveFromASC on the entry to undo every grant atomically on unequip.
     * Server-only — ability replication handles client-side ability state.
     */
    TMap<FGameplayTag, FRiftAbilitySetGrantedHandles> GrantedAbilitySetHandles;

    /**
     * Stores the primary hand ARiftWeaponActor spawned for each weapon slot.
     * Keyed by slot tag. Server-only — actors replicate normally.
     */
    UPROPERTY()
    TMap<FGameplayTag, TObjectPtr<ARiftWeaponActor>> SpawnedWeaponActors;

    /**
     * Stores the off-hand ARiftWeaponActor spawned for each weapon slot (shield or two-handed grip).
     * Keyed by slot tag. Server-only — actors replicate normally.
     */
    UPROPERTY()
    TMap<FGameplayTag, TObjectPtr<ARiftWeaponActor>> SpawnedOffHandActors;

    /**
     * Resolves the owning pawn's PlayerState and the linked inventory.
     * May reschedule itself for the next tick if the PlayerState is not yet available
     * (this happens on listen-server / local play when BeginPlay races possession).
     */
    void OnPawnReady();

    /**
     * Called back by URiftInventoryComponent::WaitForInitialized once the inventory
     * has finished its async initialization. Binds the item-event listener.
     *
     * @param bSuccess  False if the inventory failed to initialize (e.g. no data asset set).
     */
    void OnInventoryReady(bool bSuccess);

    /**
     * Listens to the linked inventory's OnItemEvent delegate.
     * Handles the case where an item is forcibly removed from inventory while still
     * equipped (e.g. an admin command strips an item). Cleans up abilities and weapon actors.
     *
     * @param Item      The item instance that triggered the event.
     * @param EventTag  The gameplay tag describing the event type.
     */
    UFUNCTION()
    void OnInventoryItemEvent(URiftItemInstance* Item, FGameplayTag EventTag);

    /**
     * Listens to the linked inventory's OnItemAdded delegate.
     * When bAutoEquipOnPickup is true, automatically equips the item if it has a
     * URiftFragment_Equippable and the matching slot is empty.
     *
     * @param Item       The item that was just added.
     * @param Container  The container it was placed in (typically backpack).
     */
    UFUNCTION()
    void OnInventoryItemAdded(URiftItemInstance* Item, URiftContainer* Container);

    /**
     * Bound to URiftInventoryComponent::OnItemMoved after the inventory is ready.
     * Detects items dragged directly into or out of equipment slot containers and
     * calls FinishEquip / FinishUnequip respectively — without moving the item again
     * since the inventory already did that.
     */
    UFUNCTION()
    void OnInventoryItemMoved(URiftItemInstance* Item, URiftContainer* FromContainer, URiftContainer* ToContainer);

    /**
     * Grants all ability sets from the item's URiftFragment_Equippable_GAS to the pawn's ASC.
     * Accumulates all resulting handles into GrantedAbilitySetHandles[SlotTag] so they can
     * be revoked atomically when the slot is unequipped. No-op for items using the base
     * URiftFragment_Equippable (non-GAS).
     *
     * @param Item     The newly equipped item.
     * @param SlotTag  Used as the key in GrantedAbilitySetHandles.
     */
    /**
     * Records the slot entry, broadcasts the equip event, grants GAS abilities, and
     * spawns the weapon actor. Does NOT move the item — callers are responsible for
     * placing the item in the slot container before or after calling this.
     *
     * Separated from EquipItem so OnInventoryItemMoved can trigger equip setup when
     * an item is dragged directly into a slot container without going through EquipItem.
     *
     * @param Item     The item being equipped.
     * @param SlotTag  The slot it is going into.
     */
    void FinishEquip(URiftItemInstance* Item, FGameplayTag SlotTag);

    /**
     * Revokes GAS grants, destroys the weapon actor, broadcasts the unequip event,
     * and removes the slot entry. Does NOT move the item back to the backpack.
     *
     * Separated from UnequipItem so OnInventoryItemMoved can trigger unequip cleanup
     * when an item is dragged out of a slot container without going through UnequipItem.
     *
     * @param Item     The item being unequipped.
     * @param SlotTag  The slot it is leaving.
     */
    void FinishUnequip(URiftItemInstance* Item, FGameplayTag SlotTag);

    void GrantItemAbilities(URiftItemInstance* Item, FGameplayTag SlotTag);

    /**
     * Revokes all GAS grants (abilities, effects, attribute sets) that were made for
     * the given slot. Calls FRiftAbilitySetGrantedHandles::RemoveFromASC on the stored
     * handle struct, then removes the slot entry from GrantedAbilitySetHandles.
     *
     * @param SlotTag  The slot whose grants should be revoked.
     */
    void RevokeItemAbilities(FGameplayTag SlotTag);

    /**
     * Spawns weapon actors for the primary and/or off-hand sockets defined on the weapon fragment.
     * Attaches each actor to the pawn's Mutable body mesh at the named socket.
     * Calls SetupForItem on each actor to apply Mutable parameters.
     *
     * @param Item     The equipped item instance.
     * @param SlotTag  Used as the key in SpawnedWeaponActors / SpawnedOffHandActors.
     * @param Fragment The weapon fragment providing socket names.
     */
    void SpawnWeaponActors(URiftItemInstance* Item, FGameplayTag SlotTag, const URiftFragment_Equippable_Weapon* Fragment);

    /**
     * Destroys the primary and off-hand weapon actors for the given slot.
     * Safe to call even if no actors were spawned (no-op in that case).
     *
     * @param SlotTag  The slot whose weapon actors should be destroyed.
     */
    void DestroyWeaponActors(FGameplayTag SlotTag);

    /**
     * Retrieves the UAbilitySystemComponent from the owner via IAbilitySystemInterface.
     * Returns nullptr if the owner does not implement the interface (should not happen in normal use).
     */
    UAbilitySystemComponent* GetASC() const;

    /**
     * Finds the USkeletalMeshComponent on the owning pawn tagged Rift.Component.BodyMesh.
     * Weapon actors attach to this component's sockets.
     * Returns nullptr if no such component exists (logs a warning at weapon spawn time).
     */
    USkeletalMeshComponent* FindBodyMeshComponent() const;
};
