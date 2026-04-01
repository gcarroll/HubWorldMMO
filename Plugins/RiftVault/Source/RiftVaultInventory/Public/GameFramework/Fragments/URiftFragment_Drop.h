#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Fragments/URiftItemFragment.h"
#include "URiftFragment_Drop.generated.h"

class UStaticMesh;

/**
 * Governs how an item behaves when the player tries to drop or delete it.
 *
 * Drop behaviour:
 *   - If this fragment is absent, the item cannot be dropped but CAN be deleted.
 *   - If bDroppable is true, URiftDropComponent spawns or finds a nearby ARiftPickup
 *     and adds the item to its inventory.
 *   - If bDroppable is false and bDeletable is true, the item can only be deleted.
 *
 * Deletion behaviour:
 *   - bDeletable controls whether URiftDropComponent::DeleteItem is permitted.
 *   - Items without this fragment are also deletable by default.
 *
 * World mesh:
 *   - DropMesh is the static mesh shown on the ARiftPickup actor in the world.
 *   - Leave unset to keep the pickup's default appearance.
 *
 * Tags contributed:
 *   - Rift.Item.Trait.Droppable  — when bDroppable is true.
 *   - Rift.Item.Trait.Deletable  — when bDeletable is true.
 */
UCLASS(DisplayName = "Drop Fragment")
class RIFTVAULTINVENTORY_API URiftFragment_Drop : public URiftItemFragment
{
    GENERATED_BODY()

public:

    /** Mesh shown on the pickup actor when this item is in the world. Leave unset for the default pickup mesh. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Drop")
    UStaticMesh* GetDropMesh() const;

    /** True if this item can be dropped into the world as a pickup. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Drop")
    FORCEINLINE bool IsDroppable() const { return bDroppable; }

    /** True if this item can be permanently deleted from the inventory. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Drop")
    FORCEINLINE bool IsDeletable() const { return bDeletable; }

    // -- Begin URiftItemFragment interface
    virtual void AppendFragmentTags(const URiftItemInstance* Item, FGameplayTagContainer& TagContainer) const override;
    // -- End URiftItemFragment interface

protected:

    /** Static mesh for the world-space pickup representation. Soft ref — loaded on demand. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drop")
    TSoftObjectPtr<UStaticMesh> DropMesh;

    /** Allow players to drop this item into the world. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drop")
    bool bDroppable = true;

    /** Allow players to permanently delete this item. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drop")
    bool bDeletable = true;
};
