# RiftVault — Advanced C++ How-To Guide

**Version:** 1.0
**Engine:** Unreal Engine 5.7+
**Author:** RiftVault Development
**Last Updated:** 2026-03-21

This guide covers RiftVault integration from C++. It assumes familiarity with Unreal's component model, GAS, and replication basics. For Blueprint-only tasks, see the Blueprint How-To Guide instead.

---

## Table of Contents

1. [Module Setup and Include Paths](#1-module-setup-and-include-paths)
2. [Accessing the Inventory Component](#2-accessing-the-inventory-component)
3. [Adding and Removing Items](#3-adding-and-removing-items)
4. [Reading Item Data](#4-reading-item-data)
5. [Working with Stacks](#5-working-with-stacks)
6. [Containers — Query and Iterate](#6-containers--query-and-iterate)
7. [Waiting for Inventory Initialization](#7-waiting-for-inventory-initialization)
8. [Binding to Inventory Delegates](#8-binding-to-inventory-delegates)
9. [Equipment — Equip and Unequip](#9-equipment--equip-and-unequip)
10. [Equipment — Querying Slot State](#10-equipment--querying-slot-state)
11. [Creating a Custom Fragment](#11-creating-a-custom-fragment)
12. [Creating a Custom Fragment State](#12-creating-a-custom-fragment-state)
13. [Broadcasting and Handling Item Events](#13-broadcasting-and-handling-item-events)
14. [Replication Patterns](#14-replication-patterns)
15. [Writing an Item Pickup Component](#15-writing-an-item-pickup-component)
16. [Implementing a Persistence Backend](#16-implementing-a-persistence-backend)
17. [Tag Reference](#17-tag-reference)
18. [Common C++ Pitfalls](#18-common-c-pitfalls)

---

## 1. Module Setup and Include Paths

Add RiftVault modules to your game module's `Build.cs`:

```csharp
// YourGame.Build.cs
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core", "CoreUObject", "Engine",
    "GameplayAbilities", "GameplayTags",
    "RiftVaultCore",
    "RiftVaultInventory",
    "RiftVaultEquipment",   // if using equipment
    "RiftVaultLoot",        // if using pickups
    "RiftVaultUI",          // if creating custom widgets
});
```

**Key include paths:**

```cpp
// Foundation
#include "Tags/RiftVaultTags.h"                              // all Tag_Rift_* globals
#include "Data/URiftContainerDefinition.h"                   // container data assets

// Inventory
#include "Components/URiftInventoryComponent.h"
#include "GameFramework/Containers/URiftContainer.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "Data/URiftItemDefinition.h"
#include "GameFramework/Fragments/URiftItemFragment.h"
#include "GameFramework/Fragments/URiftFragment_Stack.h"
#include "GameFramework/Fragments/URiftFragment_Display.h"
#include "GameFramework/Fragments/URiftFragment_Equippable.h"
#include "GameFramework/Fragments/URiftFragment_Drop.h"

// Equipment
#include "Components/URiftEquipmentComponent.h"
```

---

## 2. Accessing the Inventory Component

The inventory lives on `APlayerState`. Always walk through the PlayerState, never cache it directly on the Pawn (the pawn can be destroyed and recreated; the PlayerState persists).

```cpp
// From a PlayerController
APlayerState* PS = PlayerController->GetPlayerState<APlayerState>();
URiftInventoryComponent* InvComp = PS
    ? PS->FindComponentByClass<URiftInventoryComponent>()
    : nullptr;

// From a Pawn
APlayerState* PS = GetPlayerState<APlayerState>();
URiftInventoryComponent* InvComp = PS
    ? PS->FindComponentByClass<URiftInventoryComponent>()
    : nullptr;

// Always guard
if (!IsValid(InvComp))
{
    // Inventory not available — may be racing with possession or be a non-player pawn
    return;
}
```

---

## 3. Adding and Removing Items

All mutations are server-only. Gate calls with `GetOwner()->HasAuthority()` or place them in server-side code paths.

```cpp
// Add 1 unit of an item type (server only)
if (GetOwner()->HasAuthority())
{
    FGuid ItemId = InvComp->AddItem(MyItemDefinition, 1);
    // ItemId is invalid if the item was not added (inventory full, definition null, etc.)
}

// Add to a specific container
URiftContainer* Backpack = InvComp->GetContainerByTag(Tag_Rift_Container_Backpack.GetTag());
if (IsValid(Backpack))
{
    InvComp->AddItemToContainer(MyItemDefinition, Backpack, 5);
}

// Remove a specific instance
InvComp->RemoveItem(MyItemInstance);

// Move to a specific slot in the same container
InvComp->MoveItemToSlot(MyItemInstance, 3);

// Move to a different container
URiftContainer* ArmorSlotContainer = InvComp->GetContainerByTag(Tag_Rift_Slot_Armor_Head.GetTag());
InvComp->MoveItemToContainer(MyItemInstance, ArmorSlotContainer);

// Move to a specific slot in a different container
InvComp->MoveItemToContainerAtSlot(MyItemInstance, ArmorSlotContainer, 0);
```

---

## 4. Reading Item Data

```cpp
// Get all items
TArray<URiftItemInstance*> AllItems = InvComp->GetAllItems();

// Find by definition
TArray<URiftItemInstance*> Potions = InvComp->GetItemsByDefinition(PotionDefinition);

// Check presence
bool bHasSword = InvComp->HasItem(SwordDefinition);

// Find item by GUID (e.g. from persistence load)
URiftItemInstance* Item = InvComp->GetItemById(SavedItemGuid);

// Read definition from instance
URiftItemDefinition* Def = Item->GetDefinition();

// Find a typed fragment on the item
const URiftFragment_Display* DisplayFrag = Item->FindFragment<URiftFragment_Display>();
if (DisplayFrag)
{
    FText Name = DisplayFrag->GetDisplayName();
    UTexture2D* Icon = DisplayFrag->GetIcon();
}

// Check if item has a fragment class at all
bool bIsEquippable = Item->HasFragmentOfClass(URiftFragment_Equippable::StaticClass());
```

---

## 5. Working with Stacks

```cpp
// Get the stack fragment from an item instance (not the definition)
const URiftFragment_Stack* StackFrag = Cast<URiftFragment_Stack>(
    Item->FindFragmentByClass(URiftFragment_Stack::StaticClass()));

if (StackFrag)
{
    // Read quantity
    int32 Qty = StackFrag->GetCurrentQuantity(Item);

    // Read max
    int32 MaxQty = StackFrag->GetMaxStackSize();

    // Add quantity — returns overflow (units that didn't fit)
    // SERVER ONLY
    int32 Overflow = StackFrag->AddQuantity(Item, 10);

    // Remove quantity — floors at 1, returns how many were actually removed
    // SERVER ONLY
    int32 Removed = StackFrag->RemoveQuantity(Item, 5);

    // Check if full
    bool bFull = StackFrag->IsFull(Item);

    // Check if two items can merge
    bool bCanMerge = StackFrag->CanMergeWith(SourceItem, TargetItem);
}

// Split a stack (server only)
FGuid NewStackId = InvComp->SplitStack(Item, 3); // moves 3 units to new instance
```

> **RemoveQuantity floors at 1.** To consume the last unit of a stack, call `InvComp->RemoveItem(Item)` rather than `RemoveQuantity(Item, 1)`. The inventory component destroys the instance and broadcasts `OnItemRemoved`.

---

## 6. Containers — Query and Iterate

```cpp
// Get all containers
TArray<URiftContainer*> Containers = InvComp->GetAllContainers();

// Find by tag
URiftContainer* Backpack = InvComp->GetContainerByTag(Tag_Rift_Container_Backpack.GetTag());

// Find by definition asset pointer (more precise when multiple containers share a tag)
URiftContainer* HeadSlot = InvComp->GetContainerByDefinition(HeadSlotDefinitionAsset);

// Find by instance GUID
URiftContainer* Container = InvComp->GetContainerById(SavedContainerGuid);

// Query a container
if (IsValid(Backpack))
{
    int32 Capacity = Backpack->GetCapacity();
    int32 UsedSlots = Backpack->CountItems();
    int32 FreeSlots = Backpack->CountAvailableSlots();
    bool bFull = Backpack->IsFull();
    bool bHasItem = Backpack->HasItem(MyItem);

    // Get item at a specific slot index
    URiftItemInstance* ItemAtSlot5 = Backpack->GetItemAtSlot(5); // null if empty

    // Find which slot an item occupies
    int32 SlotIndex = Backpack->GetSlotIndexOfItem(MyItem); // INDEX_NONE if not in this container

    // Get all valid items (skips null/empty slots)
    TArray<URiftItemInstance*> AllItemsInBag = Backpack->GetAllItems();

    // Check if an item would be accepted
    bool bCanAdd = Backpack->CanAcceptItem(MyItem);
}
```

---

## 7. Waiting for Inventory Initialization

The inventory component initializes asynchronously. Use `WaitForInitialized` for C++ callers so you handle both the "already done" and "not yet done" cases correctly.

```cpp
// In your component's BeginPlay or OnPawnReady equivalent
void UMyEquipmentHelper::BeginPlay()
{
    Super::BeginPlay();

    APlayerState* PS = GetOwner<APawn>()->GetPlayerState<APlayerState>();
    if (!PS) { return; }

    URiftInventoryComponent* InvComp = PS->FindComponentByClass<URiftInventoryComponent>();
    if (!InvComp) { return; }

    // Lambda fires immediately if already initialized, or queued until it completes
    InvComp->WaitForInitialized(FOnRiftInventoryInitializedDelegate::CreateUObject(
        this, &UMyEquipmentHelper::OnInventoryReady));
}

void UMyEquipmentHelper::OnInventoryReady(bool bSuccess)
{
    if (!bSuccess) { return; }

    // Safe to read inventory data here
}
```

For Blueprint callers, the equivalent is `K2_WaitForInitialized`.

---

## 8. Binding to Inventory Delegates

```cpp
void UMySystem::BeginPlay()
{
    Super::BeginPlay();

    // Get the inventory component (see Section 2)
    if (URiftInventoryComponent* InvComp = GetInventoryComponent())
    {
        InvComp->OnItemAdded.AddDynamic(this, &UMySystem::HandleItemAdded);
        InvComp->OnItemRemoved.AddDynamic(this, &UMySystem::HandleItemRemoved);
        InvComp->OnItemMoved.AddDynamic(this, &UMySystem::HandleItemMoved);
        InvComp->OnContainerAdded.AddDynamic(this, &UMySystem::HandleContainerAdded);
    }

    // Bind to equipment events on the pawn's equipment component
    if (URiftEquipmentComponent* EquipComp = GetOwner()->FindComponentByClass<URiftEquipmentComponent>())
    {
        EquipComp->OnItemEquipped.AddDynamic(this, &UMySystem::HandleItemEquipped);
        EquipComp->OnItemUnequipped.AddDynamic(this, &UMySystem::HandleItemUnequipped);
    }
}

// Delegate signatures must match exactly
UFUNCTION()
void UMySystem::HandleItemAdded(URiftItemInstance* Item, URiftContainer* Container) { }

UFUNCTION()
void UMySystem::HandleItemEquipped(URiftItemInstance* Item, FGameplayTag SlotTag) { }
```

> On the client, `OnItemAdded` fires via `URiftContainer::OnRep_Items` after the server replicates the Items array. This is the mechanism that drives UI updates after world pickups — no explicit RPC is needed.

---

## 9. Equipment — Equip and Unequip

Equip and unequip are server-only operations on `URiftEquipmentComponent`. The standard path is via the GAS ability system, but you can also call them directly from server-side C++.

```cpp
// Direct C++ call (server only)
URiftEquipmentComponent* EquipComp = Pawn->FindComponentByClass<URiftEquipmentComponent>();
if (IsValid(EquipComp) && Pawn->HasAuthority())
{
    // Equip — item must already be in the linked inventory
    bool bEquipped = EquipComp->EquipItem(MyItemInstance, Tag_Rift_Slot_Armor_Head.GetTag());

    // Unequip — just the slot tag, no item reference needed
    bool bUnequipped = EquipComp->UnequipItem(Tag_Rift_Slot_Armor_Head.GetTag());
}
```

**Via GAS (standard path from any code, including client-triggered):**

```cpp
// Works from client — URiftAbility_Equip is ServerOnly and handles authority
FGameplayEventData Payload;
Payload.OptionalObject = MyItemInstance;
Payload.TargetTags.AddTag(Tag_Rift_Slot_Armor_Head.GetTag());

UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
    OwningPawn,
    Tag_Rift_Ability_Equip.GetTag(),
    Payload);
```

---

## 10. Equipment — Querying Slot State

These queries read from the replicated `FRiftEquipmentSlotList` and are safe on both server and clients.

```cpp
URiftEquipmentComponent* EquipComp = Pawn->FindComponentByClass<URiftEquipmentComponent>();
if (!IsValid(EquipComp)) { return; }

// Get item in a slot (null if empty)
// On non-owning clients, Item will be null (inventory is COND_OwnerOnly)
// Use GetDefinitionForSlot to get definition on all clients
URiftItemInstance* HeadItem = EquipComp->GetItemInSlot(Tag_Rift_Slot_Armor_Head.GetTag());

// Check if occupied
bool bArmed = EquipComp->IsSlotOccupied(Tag_Rift_Slot_Weapon_Primary.GetTag());

// Get item definition for a slot — works on ALL clients, not just owner
// Use this in Mutable components on simulated proxies
URiftItemDefinition* Def = EquipComp->GetDefinitionForSlot(Tag_Rift_Slot_Armor_Head.GetTag());

// Get all equipped items
TArray<URiftItemInstance*> Equipped = EquipComp->GetAllEquippedItems();

// Check if a slot tag is valid for this pawn
bool bSlotSupported = EquipComp->IsSlotSupported(Tag_Rift_Slot_Armor_Head.GetTag());
```

---

## 11. Creating a Custom Fragment

Subclass `URiftItemFragment` in `RiftVaultInventory` (or a module that depends on it). Fragments define **type-level** data shared across all instances of the item type.

```cpp
// MyFragment.h
#pragma once

#include "GameFramework/Fragments/URiftItemFragment.h"
#include "MyFragment.generated.h"

UCLASS(DisplayName = "My Custom Fragment")
class MYGAME_API UMyFragment : public URiftItemFragment
{
    GENERATED_BODY()

public:

    UMyFragment();

    // Type-level data — same for all instances of this item type
    UFUNCTION(BlueprintPure, Category = "MyGame|Fragments")
    FORCEINLINE float GetDamageBonus() const { return DamageBonus; }

    // Override lifecycle methods if needed
    virtual void InitializeState_Implementation(URiftItemInstance* Item) override;
    virtual void ActivateForItem_Implementation(URiftItemInstance* Item) override;
    virtual void DeactivateForItem_Implementation(URiftItemInstance* Item) override;
    virtual void HandleItemEvent_Implementation(URiftItemInstance* Item, FGameplayTag EventTag) override;

protected:

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MyFragment")
    float DamageBonus = 0.f;
};
```

```cpp
// MyFragment.cpp
#include "MyFragment.h"
#include "GameFramework/Items/URiftItemInstance.h"

UMyFragment::UMyFragment()
{
    // Declare which gameplay events this fragment cares about
    // Only these tags will route HandleItemEvent to this fragment
    WatchedEventTags.AddTag(Tag_Rift_Event_Item_Equipped.GetTag());
}

void UMyFragment::InitializeState_Implementation(URiftItemInstance* Item)
{
    // Create per-instance state here if needed (see Section 12)
    // If you have no per-instance state, leave this as Super:: or empty
}

void UMyFragment::ActivateForItem_Implementation(URiftItemInstance* Item)
{
    // Called when the item becomes active in inventory (server only)
}

void UMyFragment::DeactivateForItem_Implementation(URiftItemInstance* Item)
{
    // Called when the item is removed from inventory (server only)
    // Clean up anything you set up in Activate
}

void UMyFragment::HandleItemEvent_Implementation(URiftItemInstance* Item, FGameplayTag EventTag)
{
    // Called when a matching event is broadcast via BroadcastItemEvent (server only)
    if (EventTag == Tag_Rift_Event_Item_Equipped.GetTag())
    {
        // Item was equipped — react here
    }
}
```

Add it to a data asset: open a `URiftItemDefinition`, click **+** under **Fragments**, and select `My Custom Fragment`.

---

## 12. Creating a Custom Fragment State

Fragment state stores **per-instance** runtime data that differs between instances of the same item type. Example: durability, stack quantity, enchantment level.

```cpp
// MyFragmentState.h — lives in RiftVaultCore or your game module
#pragma once

#include "Types/State/FRiftFragmentState.h"
#include "MyFragmentState.generated.h"

USTRUCT(BlueprintType)
struct FMyFragmentState : public FRiftFragmentState
{
    GENERATED_BODY()

    // Per-instance data
    UPROPERTY()
    float CurrentCharge = 100.f;

    UPROPERTY()
    bool bIsCharged = true;
};
```

In your fragment's `InitializeState_Implementation`:

```cpp
void UMyFragment::InitializeState_Implementation(URiftItemInstance* Item)
{
    // Create and save the initial state for this item instance
    // Only called server-side
    TInstancedStruct<FRiftFragmentState> State;
    State.InitializeAs<FMyFragmentState>();

    FMyFragmentState& S = State.GetMutable<FMyFragmentState>();
    S.CurrentCharge = 100.f;
    S.bIsCharged = true;

    Item->SaveStateForFragment(UMyFragment::StaticClass(), State);
}
```

Reading state in any fragment method:

```cpp
TInstancedStruct<FRiftFragmentState> StateStruct;
if (Item->GetStateForFragment(UMyFragment::StaticClass(), StateStruct))
{
    const FMyFragmentState& State = StateStruct.Get<FMyFragmentState>();
    float Charge = State.CurrentCharge;
}
```

Writing state (server only):

```cpp
if (Item->HasAuthority())
{
    TInstancedStruct<FRiftFragmentState> StateStruct;
    if (Item->GetStateForFragment(UMyFragment::StaticClass(), StateStruct))
    {
        FMyFragmentState& State = StateStruct.GetMutable<FMyFragmentState>();
        State.CurrentCharge -= DrainAmount;
        Item->SaveStateForFragment(UMyFragment::StaticClass(), StateStruct);
    }
}
```

---

## 13. Broadcasting and Handling Item Events

The inventory component routes gameplay events to fragments via `BroadcastItemEvent`. Use this to decouple systems — instead of calling fragment methods directly, broadcast a tag and let fragments opt-in via `WatchedEventTags`.

**Broadcasting an event (server only):**

```cpp
// Signal that the item was equipped — all fragments watching Tag_Rift_Event_Item_Equipped receive it
InvComp->BroadcastItemEvent(Item, Tag_Rift_Event_Item_Equipped.GetTag());

// Custom event (use a tag under Rift.Event namespace)
InvComp->BroadcastItemEvent(Item, Tag_Rift_Event_Item_StackChanged.GetTag());
```

**Opting in to an event in a fragment:**

```cpp
UMyFragment::UMyFragment()
{
    // Declare in constructor — no other setup needed
    WatchedEventTags.AddTag(Tag_Rift_Event_Item_Equipped.GetTag());
}

void UMyFragment::HandleItemEvent_Implementation(URiftItemInstance* Item, FGameplayTag EventTag)
{
    // This fires when BroadcastItemEvent is called with a matching tag
}
```

**Listening to item events on an external system (e.g. a component):**

```cpp
InvComp->OnItemEvent.AddDynamic(this, &UMySystem::HandleItemEvent);

UFUNCTION()
void UMySystem::HandleItemEvent(URiftItemInstance* Item, FGameplayTag EventTag)
{
    if (EventTag == Tag_Rift_Event_Item_Equipped.GetTag())
    {
        // React to equip
    }
}
```

---

## 14. Replication Patterns

### What replicates and to whom

| Object | Replication | Audience |
|---|---|---|
| `URiftInventoryComponent.Containers` | COND_OwnerOnly | Owning client only |
| `URiftInventoryComponent.Items` | COND_OwnerOnly | Owning client only |
| `URiftContainer.Items` | `ReplicatedUsing = OnRep_Items`, COND_OwnerOnly | Owning client only |
| `URiftItemInstance.ItemId` / `.ItemDefinition` | Replicated | Owning client only |
| `FRiftEquipmentSlotList` | FastArray | All clients |
| `ARiftWeaponActor` | Normal actor replication | All clients |

### OnRep_Items — client UI updates after server-side mutations

`URiftContainer::OnRep_Items` fires on the owning client when the `Items` array replicates. It diffs the new array against `PreviousItems` and broadcasts `OnItemAdded` / `OnItemRemoved` on the inventory component so widgets update without server-to-client RPCs.

```cpp
void URiftContainer::OnRep_Items()
{
    URiftInventoryComponent* InvComp = Cast<URiftInventoryComponent>(GetOuter());
    if (!IsValid(InvComp)) { PreviousItems = Items; return; }

    // Items in new array but not in previous → added
    for (const TObjectPtr<URiftItemInstance>& Item : Items)
    {
        if (IsValid(Item) && !PreviousItems.Contains(Item))
            InvComp->OnItemAdded.Broadcast(Item.Get(), this);
    }

    // Items in previous array but not in new → removed
    for (const TObjectPtr<URiftItemInstance>& OldItem : PreviousItems)
    {
        if (IsValid(OldItem) && !Items.Contains(OldItem))
            InvComp->OnItemRemoved.Broadcast(OldItem.Get(), this);
    }

    PreviousItems = Items;
}
```

### Client-side prediction for drag-drop

When a client widget moves an item cross-container (drag-drop), it calls the mutation locally first for immediate visual feedback, then sends a Server RPC:

```cpp
// In URiftItemSlotWidget::OnItemDropped_Implementation
// Local call — UI updates immediately
InventoryComponent->MoveItemToContainerAtSlot(DraggedItem, Container.Get(), SlotIndex);

// Server RPC — authority mutation, broadcasts OnItemMoved on server
// Equipment component's OnInventoryItemMoved fires here → FinishEquip/FinishUnequip
InventoryComponent->Server_MoveItemToContainerAtSlot(DraggedItem, Container.Get(), SlotIndex);
```

The server's replicated result overwrites the client's local state if they differ.

### Replicating custom subobjects

If you add a new `UObject` subobject (similar to `URiftContainer`) that needs to replicate, follow this pattern:

```cpp
// In the owning component's header
UPROPERTY(Replicated)
TObjectPtr<UMySubobject> MySubobject;

// In GetLifetimeReplicatedProps
DOREPLIFETIME_CONDITION(UMyOwnerComp, MySubobject, COND_OwnerOnly);

// In ReplicateSubobjects
bWroteSomething |= Channel->ReplicateSubobject(MySubobject, *Bunch, *RepFlags);

// In the subobject class
virtual bool IsSupportedForNetworking() const override { return true; }
virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
```

---

## 15. Writing an Item Pickup Component

The built-in `URiftPickupComponent` handles the common case. For a custom pickup system (e.g. interactable chests, proximity looting):

```cpp
// MyPickupInteractor.h
UCLASS(ClassGroup=(MyGame), meta=(BlueprintSpawnableComponent))
class MYGAME_API UMyPickupInteractor : public UActorComponent
{
    GENERATED_BODY()

public:

    // Call this when the player interacts with the pickup actor
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "MyGame")
    void CollectItem(APawn* Collector);

    UPROPERTY(EditDefaultsOnly, Category = "Pickup")
    TObjectPtr<URiftItemDefinition> ItemDefinition;

    UPROPERTY(EditDefaultsOnly, Category = "Pickup")
    int32 Quantity = 1;
};
```

```cpp
// MyPickupInteractor.cpp
void UMyPickupInteractor::CollectItem(APawn* Collector)
{
    if (!GetOwner()->HasAuthority() || !IsValid(Collector)) { return; }

    APlayerState* PS = Collector->GetPlayerState<APlayerState>();
    if (!IsValid(PS)) { return; }

    URiftInventoryComponent* InvComp = PS->FindComponentByClass<URiftInventoryComponent>();
    if (!IsValid(InvComp)) { return; }

    FGuid AddedItemId = InvComp->AddItem(ItemDefinition, Quantity);
    if (AddedItemId.IsValid())
    {
        // Success — optionally destroy actor, play VFX, etc.
        GetOwner()->Destroy();
    }
    else
    {
        // Inventory full or item rejected — give feedback to player
    }
}
```

---

## 16. Implementing a Persistence Backend

RiftVault's persistence system is behind `IRiftPersistenceInterface` (in `RiftVaultSave`). Implement this interface in your game's backend class and assign it to the inventory component before `BeginPlay`.

```cpp
// MyInventoryBackend.h
#pragma once

#include "Interfaces/IRiftPersistenceInterface.h"
#include "MyInventoryBackend.generated.h"

UCLASS()
class MYGAME_API UMyInventoryBackend : public UObject, public IRiftPersistenceInterface
{
    GENERATED_BODY()

public:

    virtual void SaveInventory(
        const FRiftInventorySaveData& Data,
        const FOnSaveComplete& OnComplete) override;

    virtual void LoadInventory(
        const FString& PlayerId,
        const FOnLoadComplete& OnComplete) override;
};
```

```cpp
// MyInventoryBackend.cpp
void UMyInventoryBackend::SaveInventory(
    const FRiftInventorySaveData& Data,
    const FOnSaveComplete& OnComplete)
{
    // Write Data.InventoryData (TArray<uint8>) to your backend
    // When done, broadcast OnComplete with bSuccess
    bool bSuccess = MyBackend::Write(Data.PlayerId, Data.InventoryData);
    OnComplete.ExecuteIfBound(bSuccess);
}

void UMyInventoryBackend::LoadInventory(
    const FString& PlayerId,
    const FOnLoadComplete& OnComplete)
{
    // Read from backend — this may be async
    TArray<uint8> RawData = MyBackend::Read(PlayerId);

    FRiftInventorySaveData SaveData;
    SaveData.PlayerId = PlayerId;
    SaveData.InventoryData = RawData;

    bool bSuccess = RawData.Num() > 0;
    OnComplete.ExecuteIfBound(bSuccess, SaveData);
}
```

**Wiring it up (in APlayerState::BeginPlay or constructor):**

```cpp
URiftInventoryComponent* InvComp = FindComponentByClass<URiftInventoryComponent>();
if (IsValid(InvComp))
{
    UMyInventoryBackend* Backend = NewObject<UMyInventoryBackend>(this);
    InvComp->SetPersistenceObject(Backend);
}
```

Must be called before `URiftInventoryComponent::BeginPlay` executes.

---

## 17. Tag Reference

All tags are declared in `RiftVaultTags.h` and accessible as global `FNativeGameplayTag` variables. Use `.GetTag()` to get the underlying `FGameplayTag`:

```cpp
// Container tags
Tag_Rift_Container_Backpack.GetTag()

// Slot tags (equipment)
Tag_Rift_Slot_Armor_Head.GetTag()
Tag_Rift_Slot_Armor_Chest.GetTag()
Tag_Rift_Slot_Weapon_Primary.GetTag()
Tag_Rift_Slot_Weapon_Secondary.GetTag()

// Item trait tags
Tag_Rift_Item_Trait_Equippable.GetTag()
Tag_Rift_Item_Trait_Stackable.GetTag()
Tag_Rift_Item_Trait_Droppable.GetTag()
Tag_Rift_Item_Trait_Deletable.GetTag()

// Item event tags
Tag_Rift_Event_Item_Equipped.GetTag()
Tag_Rift_Event_Item_Unequipped.GetTag()
Tag_Rift_Event_Item_StackChanged.GetTag()
Tag_Rift_Event_Item_Removed.GetTag()

// Ability activation tags (send via SendGameplayEventToActor)
Tag_Rift_Ability_Equip.GetTag()
Tag_Rift_Ability_Unequip.GetTag()

// Component identification tag (FName string form: "Rift.Component.BodyMesh")
Tag_Rift_Component_BodyMesh.GetTag().GetTagName()   // used in ComponentTags lookup
```

> `FNativeGameplayTag` does not have `GetTagName()` directly. Always call `.GetTag()` first to get `FGameplayTag`, then call `GetTagName()` on that.

---

## 18. Common C++ Pitfalls

### UFunctions cannot take TObjectPtr as parameters
UHT rejects `TObjectPtr<T>` as UFUNCTION parameter or return type. Use raw pointers (`T*`) in all UFUNCTION signatures. Only `UPROPERTY` members can use `TObjectPtr`.

### RepNotify functions cannot take TObjectPtr arrays as parameters
Even `const TArray<TObjectPtr<T>>&` is rejected by UHT in UFUNCTION declarations. Use the no-parameter form and store previous state as a non-replicated UPROPERTY member.

### BlueprintAuthorityOnly does not prevent C++ calls
`UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)` only guards Blueprint graph callers. C++ can call these functions from any machine. Add explicit `GetOwner()->HasAuthority()` guards in C++ if needed.

### HasAuthority() is on AActor, not UActorComponent
From a component, use `GetOwner()->HasAuthority()` — not `HasAuthority()` directly. Components do not inherit the `HasAuthority()` shortcut.

### const TArray<T>& not supported as UFUNCTION return type
UHT error: "Inappropriate keyword 'const' on variable of type 'TArray'". Remove the `UFUNCTION` macro. C++ callers work fine. For Blueprint access, return by value `TArray<T>`.

### FNativeGameplayTag::GetTagName() does not exist
Use `.GetTag().GetTagName()` — `.GetTag()` returns `FGameplayTag`, which has `GetTagName()`.

### OnItemMoved only fires on the machine that called MoveItemToContainerAtSlot
If drag-drop calls the function client-side only, the server's `OnInventoryItemMoved` delegate never fires and equip/unequip setup is skipped. Always pair local calls with `Server_MoveItemToContainerAtSlot`.

### Mutable UpdateSkeletalMeshAsync ignores concurrent calls
Calling `UpdateSkeletalMeshAsync` while a previous generation is in progress silently discards the second call. Use `ScheduleMutableUpdate` / `FlushMutableUpdate` timer pattern to coalesce multiple parameter writes in the same tick into one call.

### Template form of FindFragment causes MSVC errors
Prefer the StaticClass form to avoid C2275/C2059:
```cpp
// Correct
Cast<URiftFragment_Stack>(Item->FindFragmentByClass(URiftFragment_Stack::StaticClass()))

// May cause MSVC errors if type is only forward declared
Item->FindFragment<URiftFragment_Stack>()  // only use when type is fully included
```

### Mutable headers need MuCO/ prefix
`#include "CustomizableSkeletalComponent.h"` → C1083. Always use:
```cpp
#include "MuCO/CustomizableSkeletalComponent.h"
#include "MuCO/CustomizableObjectInstance.h"
```

### FGameplayEventData::OptionalObject is const
`TriggerEventData->OptionalObject` returns `const UObject*`. Cast preserves const:
```cpp
// Need non-const URiftItemInstance*
URiftItemInstance* Item = const_cast<URiftItemInstance*>(
    Cast<URiftItemInstance>(TriggerEventData->OptionalObject));
```
