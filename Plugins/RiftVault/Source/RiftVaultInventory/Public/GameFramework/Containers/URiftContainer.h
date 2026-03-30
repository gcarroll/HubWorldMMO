#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Data/URiftContainerDefinition.h"
#include "Data/URiftItemDefinition.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "URiftContainer.generated.h"

class URiftInventoryComponent;
class URiftContainer;

// ------------------------------------------------------------------
// Slot fast-array types
// ------------------------------------------------------------------

/**
 * One slot entry in a container's slot list.
 *
 * Replication design:
 *   - ItemDefinition and Quantity are the replicated wire descriptor (~8 bytes per dirty slot).
 *   - Item is a local cache populated by the owning client during reconstruction
 *     and by the server when the item is placed. It is NOT replicated.
 *   - Quantity == 0 means the slot is empty.
 *
 * On clients, FRiftSlotList::PostReplicatedAdd/Change reconstruct a URiftItemInstance
 * locally from (ItemDefinition, Quantity) via URiftInventoryComponent::ReconstructItemInstance.
 * No URiftItemInstance subobject replication is used.
 */
USTRUCT()
struct FRiftSlotEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    /** Item definition — replicated wire descriptor. Null when the slot is empty. */
    UPROPERTY()
    TObjectPtr<URiftItemDefinition> ItemDefinition;

    /** Stack quantity — replicated. 0 = empty slot. */
    UPROPERTY()
    int32 Quantity = 0;

    /**
     * Local item instance cache. NOT replicated.
     * Server: the live URiftItemInstance placed in this slot.
     * Owning client: reconstructed locally from (ItemDefinition, Quantity).
     * Non-owning clients: always null (they have no inventory replication).
     */
    UPROPERTY(NotReplicated)
    TObjectPtr<URiftItemInstance> Item;
};

/**
 * Fast-array wrapper for a container's slot array.
 * Callbacks fire OnItemAdded / OnItemRemoved on the owning inventory component
 * so the grid widget updates correctly without a full-array diff.
 */
USTRUCT()
struct FRiftSlotList : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FRiftSlotEntry> Entries;

    // Set by URiftContainer constructor — not replicated, valid on both server and client.
    URiftContainer* Owner = nullptr;

    void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
    void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams);
};

template<>
struct TStructOpsTypeTraits<FRiftSlotList> : TStructOpsTypeTraitsBase2<FRiftSlotList>
{
    enum { WithNetDeltaSerializer = true };
};

/**
 * Represents one container in a RiftVault inventory.
 *
 * URiftContainer is the runtime object for a container. It holds a flat ordered
 * array of URiftItemInstance pointers up to the capacity defined by its
 * URiftContainerDefinition. All items are 1x1 — there is no spatial placement.
 *
 * Containers are owned by URiftInventoryComponent on APlayerState and identified
 * by the gameplay tag on their definition (e.g. Tag_Rift_Container_Backpack).
 *
 * URiftInventoryComponent is the only class that should create containers or
 * directly mutate the Items array. External code queries containers but routes
 * all mutations through the inventory component's processing queue.
 *
 * Replication:
 *   - IsSupportedForNetworking() returns true so the engine treats this UObject
 *     as a replicated subobject.
 *   - ContainerDefinition and Items replicate to the owning client only
 *     (COND_OwnerOnly) via URiftInventoryComponent::ReplicateSubobjects.
 *   - The slot layout (Items array) is replicated as-is — null elements represent
 *     empty slots and are preserved across replication.
 */
UCLASS(BlueprintType, Blueprintable)
class RIFTVAULTINVENTORY_API URiftContainer : public UObject
{
    GENERATED_BODY()

public:

    /** Grants URiftInventoryComponent private access so it can call AddItem/RemoveItem/SetDefinition. */
    friend URiftInventoryComponent;

    URiftContainer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // -- Begin UObject interface
    /** Declares ContainerDefinition and Items for replication (COND_OwnerOnly). */
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    /** Required for the engine to replicate this UObject as a subobject via ReplicateSubobjects. */
    virtual bool IsSupportedForNetworking() const override { return true; }
    // -- End UObject interface

    /** Returns the unique ID for this container instance. Assigned once by the inventory component. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    FORCEINLINE FGuid GetContainerId() const { return ContainerId; }

    /** Returns the definition that configures this container's capacity and item filter. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    FORCEINLINE URiftContainerDefinition* GetDefinition() const { return ContainerDefinition; }

    /**
     * Returns the gameplay tag identifying this container (e.g. Rift.Container.Backpack).
     * Delegates to ContainerDefinition — returns EmptyTag if no definition is set.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    FGameplayTag GetContainerTag() const;

    /**
     * Returns the maximum number of items this container can hold.
     * Delegates to ContainerDefinition — returns 0 if no definition is set.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    int32 GetCapacity() const;

    /**
     * Returns the number of valid (non-null) item instances currently in this container.
     * Counts only occupied slots — null entries from RemoveItem are not counted.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    int32 CountItems() const;

    /**
     * Returns the number of empty slots remaining: Capacity - CountItems.
     * May be negative only if Capacity is misconfigured — treated as 0 by callers.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    int32 CountAvailableSlots() const;

    /** Returns true if this container has at least one empty slot. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    bool HasAvailableSlot() const;

    /** Returns true if this container is at capacity (CountAvailableSlots <= 0). */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    bool IsFull() const;

    /**
     * Returns true if the given item instance is in this container.
     * Implemented via GetSlotIndexOfItem — O(n) over the slot array.
     *
     * @param Item  The item to search for.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    bool HasItem(const URiftItemInstance* Item) const;

    /**
     * Returns true if an item with the given instance would be accepted by this container.
     * Checks:
     *   1. Item and definition validity.
     *   2. Container is not full.
     *   3. The item's gameplay tags satisfy the definition's AcceptsItemWithTags query.
     *
     * @param Item  The item to test.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    bool CanAcceptItem(const URiftItemInstance* Item) const;

    /**
     * Returns all valid (non-null) item instances currently in this container.
     * Null slot entries are skipped — the returned array may be shorter than Capacity.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    TArray<URiftItemInstance*> GetAllItems() const;

    /**
     * Returns the item at the given slot index, or null if the slot is empty or out of range.
     *
     * @param SlotIndex  Zero-based slot index. Invalid indices return null.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    URiftItemInstance* GetItemAtSlot(int32 SlotIndex) const;

    /**
     * Returns the slot index of the given item, or INDEX_NONE if not found.
     * O(n) linear search over the Items array.
     *
     * @param Item  The item to locate.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    int32 GetSlotIndexOfItem(const URiftItemInstance* Item) const;

    /**
     * Per-slot tag requirements for this container.
     *
     * When non-empty, an item moving to slot index i must carry the tag at index i
     * (if that tag is valid). Used by the Equipment container so that each slot only
     * accepts items whose EquipmentSlotTag matches the slot's expected position.
     *
     * Populated at runtime by URiftEquipmentComponent — not replicated, server only.
     */
    UPROPERTY()
    TArray<FGameplayTag> SlotTagRequirements;

private:

    /**
     * Unique identifier for this container instance.
     * Assigned once by URiftInventoryComponent::CreateContainer. Never changes.
     * Not replicated — clients identify containers by their definition tag.
     */
    UPROPERTY()
    FGuid ContainerId;

    /**
     * The definition that configures this container's rules and capacity.
     * Replicated COND_OwnerOnly so the owning client can read the tag and capacity.
     */
    UPROPERTY(Replicated)
    TObjectPtr<URiftContainerDefinition> ContainerDefinition;

    /**
     * The slot array for this container. Index = slot number. Entries with a null Item
     * represent empty slots — null entries are preserved rather than compacted so slot
     * identity is stable across replication. Pre-allocated to Capacity by SetDefinition.
     * Replicated to the owning client via COND_OwnerOnly using delta serialization.
     * FRiftSlotList callbacks fire OnItemAdded / OnItemRemoved on the inventory component.
     */
    UPROPERTY(Replicated)
    FRiftSlotList Slots;

    /**
     * Updates a slot's descriptor fields (Item, ItemDefinition, Quantity) and marks
     * the entry dirty for replication. Passing nullptr clears the slot.
     *
     * @param SlotIndex  The slot to update.
     * @param Item       The item to write, or nullptr to clear.
     */
    void RefreshSlotEntry(int32 SlotIndex, URiftItemInstance* Item);

    /**
     * Adds an item to the first available slot.
     * Scans for null entries first; appends if the array hasn't reached Capacity yet.
     * Private — all external additions go through URiftInventoryComponent.
     *
     * @param Item  The item to place. Must be valid.
     * @return  The slot index where the item was placed, or INDEX_NONE if full.
     */
    int32 AddItem(URiftItemInstance* Item);

    /**
     * Removes an item from this container by nulling its slot entry.
     * Does NOT compact the array — the null slot is reusable by AddItem.
     * Private — all external removals go through URiftInventoryComponent.
     *
     * @param Item  The item to remove.
     * @return  true if the item was found and its slot nulled.
     */
    bool RemoveItem(URiftItemInstance* Item);

    /**
     * Moves an item to a specific slot index by swapping with whatever is there.
     * If the target slot is occupied the two items exchange positions.
     * Private — called by URiftInventoryComponent::MoveItemToSlot.
     *
     * @param Item             The item to relocate.
     * @param TargetSlotIndex  The destination slot (must be a valid index).
     * @return  true if the swap was performed.
     */
    bool MoveItemToSlot(URiftItemInstance* Item, int32 TargetSlotIndex);

    /**
     * Places an item directly into the given slot without a swap.
     * The slot must already be empty — call RemoveItem first if needed.
     * Private — called by URiftInventoryComponent::MoveItemToContainerAtSlot.
     *
     * @param Item       The item to place. Must be valid.
     * @param SlotIndex  The destination slot (must be valid and empty).
     * @return  true if the item was placed.
     */
    bool PlaceItemAtSlot(URiftItemInstance* Item, int32 SlotIndex);

    /**
     * Sets the definition and pre-allocates the Items array to Capacity.
     * Called once by URiftInventoryComponent::CreateContainer during initialization.
     *
     * @param NewDefinition  The container definition to apply.
     */
    void SetDefinition(URiftContainerDefinition* NewDefinition);

    /**
     * Sets the unique ID. Called once by URiftInventoryComponent::CreateContainer.
     * Never changes after initial assignment.
     *
     * @param NewContainerId  The guid to assign.
     */
    void SetContainerId(const FGuid& NewContainerId);
};
