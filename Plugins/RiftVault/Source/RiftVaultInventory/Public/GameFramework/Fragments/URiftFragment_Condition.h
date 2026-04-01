#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Fragments/URiftItemFragment.h"
#include "Types/State/FRiftConditionState.h"
#include "URiftFragment_Condition.generated.h"

/**
 * Adds condition (durability) tracking to any item type.
 *
 * Add this fragment to a URiftItemDefinition to give items a condition value
 * that can degrade and be repaired. Items with zero condition are marked broken
 * and cannot be equipped until repaired.
 *
 * Data split:
 *   - Type-level data (MaxCondition, bStartAtFullCondition) lives here on the
 *     fragment, shared across all instances of the item type.
 *   - Per-instance data (CurrentCondition, bIsBroken) lives in FRiftConditionState,
 *     stored individually on each URiftItemInstance via the FragmentStates list.
 *
 * Mutation methods are server-only. Gate all calls with HasAuthority() or use
 * BlueprintAuthorityOnly UFUNCTIONs from Blueprint.
 *
 * Example usage:
 *   URiftFragment_Condition* CondFrag = Item->FindFragment<URiftFragment_Condition>();
 *   if (CondFrag)
 *   {
 *       CondFrag->ApplyDamage(Item, 10.f);
 *   }
 */
UCLASS(DisplayName = "Condition Fragment")
class RIFTVAULTINVENTORY_API URiftFragment_Condition : public URiftItemFragment
{
    GENERATED_BODY()

public:

    URiftFragment_Condition();

    // -- Begin URiftItemFragment interface
    /**
     * Creates the initial FRiftConditionState for a new item instance.
     * CurrentCondition is set to MaxCondition if bStartAtFullCondition is true,
     * otherwise it is set to 0.
     */
    virtual void InitializeState_Implementation(URiftItemInstance* Item) override;
    // -- End URiftItemFragment interface

    // ------------------------------------------------------------------
    // Type-level queries (no item instance needed)
    // ------------------------------------------------------------------

    /** Returns the maximum condition value for this item type. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Condition")
    FORCEINLINE float GetMaxCondition() const { return MaxCondition; }

    // ------------------------------------------------------------------
    // Instance-level queries (read from item state)
    // ------------------------------------------------------------------

    /**
     * Returns the current condition of the given item instance.
     * Returns 0 if the state is missing.
     *
     * @param Item  The item to query.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Condition")
    float GetCurrentCondition(const URiftItemInstance* Item) const;

    /**
     * Returns the current condition as a normalized value in [0, 1].
     * 1.0 = full condition, 0.0 = broken.
     * Returns 0 if MaxCondition is zero or state is missing.
     *
     * @param Item  The item to query.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Condition")
    float GetConditionPercent(const URiftItemInstance* Item) const;

    /**
     * Returns true if this item is currently broken (condition == 0).
     *
     * @param Item  The item to check.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Condition")
    bool IsBroken(const URiftItemInstance* Item) const;

    // ------------------------------------------------------------------
    // Instance-level mutations (server only)
    // ------------------------------------------------------------------

    /**
     * Reduces the item's condition by the given amount.
     * If condition reaches 0, bIsBroken is set to true and
     * Tag_Rift_Event_Item_ConditionChanged is broadcast.
     * Has no effect if the item is already broken.
     *
     * Must only be called server-side.
     *
     * @param Item    The item to damage.
     * @param Amount  Amount of condition to remove. Must be > 0.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Fragments|Condition")
    void ApplyDamage(URiftItemInstance* Item, float Amount);

    /**
     * Restores the item's condition by the given amount, clamped to MaxCondition.
     * If condition rises above 0 and the item was broken, bIsBroken is cleared and
     * Tag_Rift_Event_Item_ConditionChanged is broadcast.
     *
     * Must only be called server-side.
     *
     * @param Item    The item to repair.
     * @param Amount  Amount of condition to restore. Must be > 0.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Fragments|Condition")
    void Repair(URiftItemInstance* Item, float Amount);

protected:

    /**
     * Maximum condition value for this item type.
     * CurrentCondition is clamped to this value during Repair.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Condition", meta = (UIMin = 1, ClampMin = 1))
    float MaxCondition = 100.f;

    /**
     * If true, new instances start at full condition (MaxCondition).
     * Set to false for items that should start damaged or at zero condition.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Condition")
    bool bStartAtFullCondition = true;

private:

    /**
     * Retrieves the FRiftConditionState from the given item's fragment state list.
     * Returns false if the item is invalid, has no state, or the state cannot be cast.
     *
     * @param Item      The item to read state from.
     * @param OutState  Populated on success.
     * @return          True if a valid FRiftConditionState was found.
     */
    bool GetConditionState(const URiftItemInstance* Item, FRiftConditionState& OutState) const;

    /**
     * Writes the given state back to the item and broadcasts Tag_Rift_Event_Item_ConditionChanged.
     *
     * @param Item   The item to write state to.
     * @param State  The updated condition state to save.
     */
    void SaveConditionState(URiftItemInstance* Item, const FRiftConditionState& State) const;
};
