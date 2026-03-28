#pragma once

#include "CoreMinimal.h"
#include "Types/State/FRiftFragmentState.h"
#include "FRiftConditionState.generated.h"

/**
 * Per-instance state for URiftFragment_Condition (durability/condition system).
 *
 * Tracks the current condition (durability) of this specific item instance.
 * The maximum condition value and decay rates are defined on URiftFragment_Condition
 * (type-level data, shared across all instances of the same item type). This
 * struct holds only the runtime values that change during gameplay.
 *
 * CONDITION SYSTEM OVERVIEW
 * -------------------------
 * Condition degrades via URiftEffect_Wear (a GameplayEffect applied on item use).
 * Condition is restored via URiftEffect_Repair (applied by a repair station or ability).
 *
 * When CurrentCondition reaches 0.0:
 *   1. The durability system sets bIsBroken = true on this struct.
 *   2. The gameplay tag Tag_Rift_Item_Trait_Broken is applied to the owning ASC.
 *   3. Equipment attempt is blocked — URiftAbility_Equip checks for that tag.
 *   4. The item remains in inventory; it can still be repaired and re-equipped.
 *
 * REPLICATION
 * -----------
 * Both CurrentCondition and bIsBroken are replicated as part of the owning
 * FFastArraySerializer entry on URiftInventoryComponent. LastCondition is
 * NotReplicated — it exists only on the client as a shadow value for
 * PostReplicatedChange delta computation (e.g. to display "item is taking
 * damage" feedback).
 */
USTRUCT(BlueprintType, DisplayName = "Condition State")
struct RIFTVAULTCORE_API FRiftConditionState : public FRiftFragmentState
{
    GENERATED_BODY()

    /**
     * Current condition of this item instance.
     *
     * Range: [0, MaxCondition] where MaxCondition is on the fragment definition.
     * 0 = broken; item cannot be equipped until repaired.
     * UIMin/ClampMin prevent negative values being set in the editor.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition State", meta = (UIMin = 0, ClampMin = 0))
    float CurrentCondition = 100.f;

    /**
     * Whether this item instance is currently broken (condition == 0).
     *
     * Set by the durability system; do not write this directly. When true,
     * Tag_Rift_Item_Trait_Broken is active on the owning ASC and equipping
     * this item is blocked until it is repaired (condition restored above 0).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition State")
    bool bIsBroken = false;

    /**
     * Shadow copy of CurrentCondition from before the most recent replication update.
     *
     * Not replicated — lives only on the client. Set by SynchronizeReplicationState()
     * after a replicated change has been broadcast. Use this in PostReplicatedChange
     * to compute how much condition changed (e.g. to drive a damage flash on the
     * condition bar in the UI).
     *
     * Example:
     *   float Delta = NewState.CurrentCondition - NewState.LastCondition;
     */
    UPROPERTY(NotReplicated)
    float LastCondition = 0.f;

    /**
     * Synchronizes LastCondition with CurrentCondition after a replication update.
     *
     * Called by the Fast Array replication system on clients once the change
     * notification has been dispatched. After this call LastCondition == CurrentCondition,
     * so delta computation must happen in the PostReplicatedChange callback
     * BEFORE SynchronizeReplicationState() is invoked.
     */
    virtual void SynchronizeReplicationState() override
    {
        LastCondition = CurrentCondition;
    }
};
