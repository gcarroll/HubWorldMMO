#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Types/EContainerType.h"
#include "Types/EContainerLayoutType.h"
#include "URiftContainerDefinition.generated.h"

/**
 * Designer-authored data asset that describes the rules for one container
 * within the RiftVault inventory system.
 *
 * DESIGN INTENT
 * -------------
 * URiftContainerDefinition is a pure data object — it is never instantiated
 * at runtime directly. URiftContainer is the runtime object; it holds a
 * reference to its definition and delegates all capacity/acceptance queries to it.
 *
 * Each logical container in a player's inventory (backpack, equipment slots,
 * stash, etc.) is identified by a GameplayTag (e.g. Rift.Container.Backpack).
 * The definition for that tag specifies:
 *   - How many item instances the container can hold (Capacity)
 *   - What types of items are allowed (ItemCompatibilityQuery)
 *   - What role the container plays (ContainerType)
 *
 * ASSET REGISTRATION
 * ------------------
 * This class inherits from UPrimaryDataAsset. GetPrimaryAssetId() returns
 * a PrimaryAssetId with Type = "RiftContainerDefinition" so the Asset Manager
 * can discover and cook these assets automatically. Ensure the Asset Manager
 * is configured to scan for this type in DefaultGame.ini.
 *
 * DATA VALIDATION
 * ---------------
 * IsDataValid() (editor-only) checks that ContainerTag is set and Capacity > 0.
 * Run "Validate Assets" in the editor or use the commandlet to catch misconfigured
 * definitions before shipping.
 */
UCLASS(BlueprintType)
class RIFTVAULTCORE_API URiftContainerDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    URiftContainerDefinition();

    // -- Begin UPrimaryDataAsset interface --

    /**
     * Returns the PrimaryAssetId used by the Asset Manager to track this asset.
     * Type is "RiftContainerDefinition"; Name is the asset's FName.
     * Overriding this ensures consistent cooking and async loading via
     * UAssetManager::LoadPrimaryAsset.
     */
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    // -- End UPrimaryDataAsset interface --

    /**
     * Returns the player-facing display name for this container.
     * Used by UI widgets to label inventory tabs (e.g. "Backpack", "Equipment").
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container Definition")
    FORCEINLINE FText GetDisplayName() const { return DisplayName; }

    /**
     * Returns the gameplay tag that uniquely identifies this container instance
     * within the inventory (e.g. Rift.Container.Backpack).
     *
     * This tag is used as the key into URiftInventoryComponent's container map.
     * It must match one of the Tag_Rift_Container_* tags declared in RiftVaultTags.h.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container Definition")
    FORCEINLINE FGameplayTag GetContainerTag() const { return ContainerTag; }

    /**
     * Returns the role this container plays in the inventory system.
     * Equipment containers apply additional slot-tag validation beyond capacity.
     * Vendor containers are read by the economy system for price computation.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container Definition")
    FORCEINLINE EContainerType GetContainerType() const { return ContainerType; }

    /** Returns the layout type (Grid or List) that drives both UI and replication behavior. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container Definition")
    FORCEINLINE EContainerLayoutType GetLayoutType() const { return LayoutType; }

    /** Returns the total number of slots in this container. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container Definition")
    FORCEINLINE int32 GetCapacity() const { return SlotCount; }

    /**
     * Returns the number of columns for Grid layout.
     * Rows are derived automatically: ceil(SlotCount / ColumnCount).
     * Defaults to 5 if unset. Not meaningful for List layout.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container Definition")
    FORCEINLINE int32 GetColumnCount() const { return ColumnCount > 0 ? ColumnCount : 5; }

    /**
     * Evaluates whether an item with the given tag container is accepted by this
     * container definition.
     *
     * If ItemCompatibilityQuery is empty, all items are accepted (returns true).
     * Otherwise, the item's tag container must satisfy the FGameplayTagQuery.
     *
     * @param ItemTags  The gameplay tags on the item instance being evaluated.
     * @return          True if the item is allowed in this container.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Container Definition")
    bool AcceptsItemWithTags(const FGameplayTagContainer& ItemTags) const;

#if WITH_EDITOR
    // -- Begin UObject editor interface --

    /**
     * Validates this asset for shipping. Checks:
     *   - ContainerTag is set to a valid Rift.Container.* tag
     *   - Capacity is at least 1
     *   - ItemCompatibilityQuery is not unintentionally empty (warning, not error)
     *
     * @param Context  Validation context that collects errors and warnings.
     * @return         Invalid if any errors were added; Valid otherwise.
     */
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;

    // -- End UObject editor interface --
#endif

protected:

    /**
     * Player-facing name for this container.
     * Displayed in inventory UI tabs (e.g. "Backpack", "Equipment Slots").
     * Defaults to "Container" via NSLOCTEXT so it is localizable.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Container Definition")
    FText DisplayName;

    /**
     * The gameplay tag that uniquely identifies this container within the inventory.
     *
     * Must be set to one of the Tag_Rift_Container_* tags declared in RiftVaultTags.h
     * (e.g. Rift.Container.Backpack). The meta Categories restriction filters the
     * tag picker in the Details panel to only show Rift.Container subtags.
     *
     * Left unset = data validation error; container will not function correctly.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Container Definition", meta = (Categories = "Rift.Container"))
    FGameplayTag ContainerTag;

    /**
     * The role this container plays in the inventory system.
     * Defaults to PlayerInventory. Equipment containers require additional
     * slot-tag matching logic in URiftEquipmentComponent.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Container Definition")
    EContainerType ContainerType = EContainerType::PlayerInventory;

    /** Whether this container uses a grid or list layout. Drives both UI and replication. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Container Definition")
    EContainerLayoutType LayoutType = EContainerLayoutType::Grid;

    /**
     * Total number of slots in this container.
     * For Grid layout, rows are derived automatically: ceil(SlotCount / ColumnCount).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Container Definition", meta = (UIMin = 1, ClampMin = 1))
    int32 SlotCount = 20;

    /**
     * Number of columns for Grid layout. Rows are derived: ceil(SlotCount / ColumnCount).
     * Defaults to 5 if unset. Not used by List layout.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Container Definition", meta = (UIMin = 1, ClampMin = 1, EditCondition = "LayoutType == EContainerLayoutType::Grid"))
    int32 ColumnCount = 5;

    /**
     * Gameplay tag query that gates which items this container will accept.
     *
     * Leave empty to accept all items (AcceptsItemWithTags returns true unconditionally).
     * Populate with a tag query to restrict by trait — for example:
     *   "AllTagsMatch: Rift.Item.Trait.Equippable" for an equipment container.
     *
     * The default constructor builds a query that accepts any item carrying
     * a Rift.Item.Trait.* tag, which covers all standard RiftVault items.
     * Designers can override this in their specific container definition assets.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Container Definition")
    FGameplayTagQuery ItemCompatibilityQuery;
};
