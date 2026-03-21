#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "Data/URiftContainerDefinition.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "URiftContainer.generated.h"

class URiftInventoryComponent;

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
 */
UCLASS(BlueprintType, Blueprintable)
class RIFTVAULTINVENTORY_API URiftContainer : public UObject
{
    GENERATED_BODY()

public:

    friend URiftInventoryComponent;

    URiftContainer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // -- Begin UObject interface
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool IsSupportedForNetworking() const override { return true; }
    // -- End UObject interface

    /** Returns the unique ID for this container instance. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    FORCEINLINE FGuid GetContainerId() const { return ContainerId; }

    /** Returns the definition that configures this container. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    FORCEINLINE URiftContainerDefinition* GetDefinition() const { return ContainerDefinition; }

    /** Returns the gameplay tag identifying this container (e.g. Rift.Container.Backpack). */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    FGameplayTag GetContainerTag() const;

    /** Returns the maximum number of items this container can hold. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    int32 GetCapacity() const;

    /** Returns the number of items currently in this container. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    int32 CountItems() const;

    /** Returns the number of empty slots remaining in this container. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    int32 CountAvailableSlots() const;

    /** Returns true if this container has at least one empty slot. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    bool HasAvailableSlot() const;

    /** Returns true if this container is at capacity. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    bool IsFull() const;

    /** Returns true if the given item instance is in this container. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    bool HasItem(const URiftItemInstance* Item) const;

    /**
     * Returns true if an item with the given tags would be accepted by this container.
     * Checks capacity and the definition's item compatibility query.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    bool CanAcceptItem(const URiftItemInstance* Item) const;

    /** Returns all item instances currently in this container. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    TArray<URiftItemInstance*> GetAllItems() const;

    /**
     * Returns the item at the given slot index, or null if the slot is empty or out of range.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    URiftItemInstance* GetItemAtSlot(int32 SlotIndex) const;

    /**
     * Returns the slot index of the given item, or INDEX_NONE if not found.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container")
    int32 GetSlotIndexOfItem(const URiftItemInstance* Item) const;

private:

    /** Unique identifier for this container instance. */
    UPROPERTY()
    FGuid ContainerId;

    /** The definition that configures this container's rules and capacity. */
    UPROPERTY(Replicated)
    TObjectPtr<URiftContainerDefinition> ContainerDefinition;

    /**
     * The flat ordered array of item instances in this container.
     * Index = slot number. Null entries represent empty slots.
     * Replicated to the owning client via URiftInventoryNetProxy.
     */
    UPROPERTY(Replicated)
    TArray<TObjectPtr<URiftItemInstance>> Items;

    /** Adds an item to the first available slot. Returns the slot index or INDEX_NONE if full. */
    int32 AddItem(URiftItemInstance* Item);

    /** Removes an item from this container. Returns true if found and removed. */
    bool RemoveItem(URiftItemInstance* Item);

    /** Moves an item to a specific slot index. Returns true if the move succeeded. */
    bool MoveItemToSlot(URiftItemInstance* Item, int32 TargetSlotIndex);

    /** Sets the definition. Called once by URiftInventoryComponent during creation. */
    void SetDefinition(URiftContainerDefinition* NewDefinition);

    /** Sets the unique ID. Called once by URiftInventoryComponent during creation. */
    void SetContainerId(const FGuid& NewContainerId);
};
