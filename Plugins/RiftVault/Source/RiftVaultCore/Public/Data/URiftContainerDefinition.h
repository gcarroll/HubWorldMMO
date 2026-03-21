#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Types/EContainerType.h"
#include "URiftContainerDefinition.generated.h"

/**
 * Defines the rules for a container in the RiftVault inventory system.
 *
 * A URiftContainerDefinition is a designer-authored DataAsset that describes
 * what a container is and what items it will accept. It is never instantiated
 * at runtime directly — URiftContainer is the runtime object that references
 * its definition and enforces its rules.
 *
 * Each container in the player's inventory is identified by a GameplayTag
 * (e.g. Tag_Rift_Container_Backpack). The definition for that container
 * specifies its capacity and the item compatibility query that gates what
 * can be placed inside it.
 */
UCLASS(BlueprintType)
class RIFTVAULTCORE_API URiftContainerDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    URiftContainerDefinition();

    // -- Begin UPrimaryDataAsset interface
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
    // -- End UPrimaryDataAsset interface

    /** Returns the display name for this container. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container Definition")
    FORCEINLINE FText GetDisplayName() const { return DisplayName; }

    /** Returns the gameplay tag that uniquely identifies this container (e.g. Rift.Container.Backpack). */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container Definition")
    FORCEINLINE FGameplayTag GetContainerTag() const { return ContainerTag; }

    /** Returns the category of this container. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container Definition")
    FORCEINLINE EContainerType GetContainerType() const { return ContainerType; }

    /** Returns the maximum number of item instances this container can hold. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container Definition")
    FORCEINLINE int32 GetCapacity() const { return Capacity; }

    /**
     * Evaluates whether an item with the given tags is accepted by this container.
     * Uses the ItemCompatibilityQuery — returns true if the query matches or is empty.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container Definition")
    bool AcceptsItemWithTags(const FGameplayTagContainer& ItemTags) const;

#if WITH_EDITOR
    // -- Begin UObject editor interface
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
    // -- End UObject editor interface
#endif

protected:

    /** Player-facing name for this container (e.g. "Backpack", "Equipment"). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Container Definition")
    FText DisplayName;

    /**
     * The gameplay tag that uniquely identifies this container within the inventory.
     * Must match one of the Tag_Rift_Container_* tags declared in RiftVaultTags.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Container Definition", meta = (Categories = "Rift.Container"))
    FGameplayTag ContainerTag;

    /** The role this container plays in the inventory system. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Container Definition")
    EContainerType ContainerType = EContainerType::PlayerInventory;

    /**
     * Maximum number of item instances this container can hold.
     * All items are 1x1 — this is a flat slot count.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Container Definition", meta = (UIMin = 1, ClampMin = 1))
    int32 Capacity = 20;

    /**
     * Query that determines which items this container will accept.
     * Leave empty to accept all items. Use tag queries to restrict by trait
     * (e.g. only items with Tag_Rift_Item_Trait_Equippable for an equipment container).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Container Definition")
    FGameplayTagQuery ItemCompatibilityQuery;
};
