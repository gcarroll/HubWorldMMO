#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Fragments/URiftItemFragment.h"
#include "Types/State/FRiftStackState.h"
#include "URiftFragment_Stack.generated.h"

/**
 * Allows an item to exist in stacks of more than one unit.
 *
 * URiftFragment_Stack adds stackability to any item type. Add this fragment to a
 * URiftItemDefinition and set MaxStackSize to define how many units can live in
 * a single inventory slot.
 *
 * Data split:
 *   - Type-level data (MaxStackSize, bShowStackCount) lives here on the fragment,
 *     shared across all instances of the item type.
 *   - Per-instance data (CurrentQuantity) lives in FRiftStackState, which is
 *     stored individually on each URiftItemInstance via the FragmentStates list.
 *
 * Stack overflow / merge logic:
 *   - AddQuantity fills the stack up to MaxStackSize and returns any overflow.
 *   - The caller (URiftInventoryComponent::AddItem) loops until overflow is zero,
 *     creating new item instances per slot as needed.
 *
 * RemoveQuantity floors at 1:
 *   - This method never reduces CurrentQuantity below 1. The inventory component
 *     is responsible for destroying the item when the stack should reach zero.
 *     This prevents ghost items with zero quantity from lingering in containers.
 *
 * Mutation methods are server-only. Gate all calls with HasAuthority() or use
 * BlueprintAuthorityOnly UFUNCTIONs from Blueprint.
 *
 * Example usage:
 *   URiftFragment_Stack* StackFrag = Cast<URiftFragment_Stack>(
 *       Item->FindFragmentByClass(URiftFragment_Stack::StaticClass()));
 *   if (StackFrag)
 *   {
 *       int32 Overflow = StackFrag->AddQuantity(Item, 5);
 *   }
 */
UCLASS(DisplayName = "Stack Fragment")
class RIFTVAULTINVENTORY_API URiftFragment_Stack : public URiftItemFragment
{
    GENERATED_BODY()

public:

    URiftFragment_Stack();

    // -- Begin URiftItemFragment interface
    /**
     * Creates the initial FRiftStackState for a new item instance with CurrentQuantity = 1.
     * URiftInventoryComponent::AddItem will then call AddQuantity to top up the stack
     * if more than one unit is being added in the same operation.
     */
    virtual void InitializeState_Implementation(URiftItemInstance* Item) override;
    // -- End URiftItemFragment interface

    // ------------------------------------------------------------------
    // Type-level queries (no item instance needed)
    // ------------------------------------------------------------------

    /**
     * Returns the maximum number of units allowed in one stack of this item type.
     * Minimum valid value is 1 (clamped in the editor). A value of 1 means items
     * occupy individual slots even though they technically have a stack fragment.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Stack")
    FORCEINLINE int32 GetMaxStackSize() const { return MaxStackSize; }

    /**
     * Returns true if the stack count should be shown as an overlay in the UI.
     * Set to false for items like currency whose count should not clutter the item slot.
     * Checked by URiftViewModel_ItemDisplay::IsStackCountVisible().
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Stack")
    FORCEINLINE bool ShouldShowStackCount() const { return bShowStackCount; }

    // ------------------------------------------------------------------
    // Instance-level queries (read from item state)
    // ------------------------------------------------------------------

    /**
     * Returns the current number of units in the given item's stack.
     * Reads from the item's FRiftStackState. Returns 0 if the state is missing
     * (this should not occur for correctly initialized items).
     *
     * @param Item  The item to query.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Stack")
    int32 GetCurrentQuantity(const URiftItemInstance* Item) const;

    /**
     * Returns how many more units can be added to this stack before it is full.
     * Computed as FMath::Max(0, MaxStackSize - CurrentQuantity).
     * Returns 0 if the state is missing or the stack is at capacity.
     *
     * @param Item  The item to query.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Stack")
    int32 GetAvailableSpace(const URiftItemInstance* Item) const;

    /**
     * Returns true if this stack cannot accept any more units (AvailableSpace == 0).
     * Used by AddItem to skip full stacks during the fill-existing-stacks pass.
     *
     * @param Item  The item to check.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Stack")
    bool IsFull(const URiftItemInstance* Item) const;

    /**
     * Returns true if the two item instances are the same type AND the target
     * stack has room to receive at least one unit. Used to determine whether a
     * drag-and-drop merge operation is valid before attempting it.
     *
     * @param Source  The item being merged into the target (units will leave Source).
     * @param Target  The item receiving units from Source.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Stack")
    bool CanMergeWith(const URiftItemInstance* Source, const URiftItemInstance* Target) const;

    // ------------------------------------------------------------------
    // Instance-level mutations (server only)
    // ------------------------------------------------------------------

    /**
     * Adds units to this item's stack up to MaxStackSize.
     * Any units that do not fit are returned as overflow.
     *
     * Step-by-step:
     *   1. Guard: authority, Amount > 0, valid item, has state.
     *   2. Compute Available = MaxStackSize - CurrentQuantity.
     *   3. ToAdd = min(Amount, Available); Overflow = Amount - ToAdd.
     *   4. Increment CurrentQuantity by ToAdd and save back to the item.
     *   5. Fire Tag_Rift_Event_Item_StackChanged via the owning inventory component.
     *   6. Return Overflow so callers can decide whether to create a new stack.
     *
     * Must only be called server-side.
     *
     * @param Item    The item instance to add units to.
     * @param Amount  Number of units to add. Must be > 0.
     * @return        Number of units that did not fit (overflow). 0 if all fit.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Fragments|Stack")
    int32 AddQuantity(URiftItemInstance* Item, int32 Amount);

    /**
     * Removes units from this item's stack. Will not go below 1.
     *
     * Design note: RemoveQuantity intentionally floors at 1. The inventory component
     * must call RemoveItem explicitly to destroy the item. This ensures that items
     * with zero quantity never linger in a container and avoids a two-step destroy
     * from a single RemoveQuantity call.
     *
     * Step-by-step:
     *   1. Guard: authority, Amount > 0, valid item, has state.
     *   2. ToRemove = min(Amount, CurrentQuantity - 1)  (never reduce below 1).
     *   3. Decrement CurrentQuantity by ToRemove and save back to the item.
     *   4. Fire Tag_Rift_Event_Item_StackChanged via the owning inventory component.
     *   5. Return ToRemove so callers know how many were actually removed.
     *
     * Must only be called server-side.
     *
     * @param Item    The item instance to remove units from.
     * @param Amount  Number of units to remove. Must be > 0.
     * @return        Number of units actually removed (may be less than Amount).
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Fragments|Stack")
    int32 RemoveQuantity(URiftItemInstance* Item, int32 Amount);

protected:

    /**
     * Maximum number of units that can exist in one stack of this item type.
     * Must be >= 1 (enforced via ClampMin meta). A value of 1 means the item is
     * not truly stackable but still has a quantity (e.g. single-use consumables).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stack", meta = (UIMin = 1, ClampMin = 1))
    int32 MaxStackSize = 1;

    /**
     * Whether to display the stack count as an overlay on the item slot in the UI.
     * Set to false for items like gold/currency where the total amount is shown
     * elsewhere and the per-slot count overlay would be redundant or confusing.
     * Checked by URiftViewModel_ItemDisplay::IsStackCountVisible().
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stack")
    bool bShowStackCount = true;

private:

    /**
     * Helper that retrieves the FRiftStackState from the given item's fragment state list.
     * Returns false if the item is invalid, has no state, or the state cannot be cast.
     * All query and mutation methods call this rather than duplicating the state-fetch logic.
     *
     * @param Item      The item to read state from.
     * @param OutState  Populated on success.
     * @return          True if a valid FRiftStackState was found and copied into OutState.
     */
    bool GetStackState(const URiftItemInstance* Item, FRiftStackState& OutState) const;
};
