#pragma once

#include "CoreMinimal.h"
#include "Types/State/FRiftFragmentState.h"
#include "FRiftConditionState.generated.h"

/**
 * Per-instance state for URiftFragment_Condition.
 *
 * Tracks the current condition (durability) of this item instance. The maximum
 * condition value is defined on the fragment (type-level data). This struct
 * holds the runtime value that degrades via URiftEffect_Wear and is restored
 * via URiftEffect_Repair.
 *
 * When CurrentCondition reaches zero, bIsBroken is set to true by the
 * durability system. A broken item cannot be equipped until repaired.
 * The tag Tag_Rift_Item_Trait_Broken is also applied to the owning ASC.
 */
USTRUCT(BlueprintType, DisplayName = "Condition State")
struct RIFTVAULTCORE_API FRiftConditionState : public FRiftFragmentState
{
    GENERATED_BODY()

    /** Current condition of this item instance. 0 = broken. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition State", meta = (UIMin = 0, ClampMin = 0))
    float CurrentCondition = 100.f;

    /** True when CurrentCondition has reached zero. Set by the durability system. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition State")
    bool bIsBroken = false;

    /** Last condition value before replication. Used for change detection on clients. */
    UPROPERTY(NotReplicated)
    float LastCondition = 0.f;

    virtual void SynchronizeReplicationState() override
    {
        LastCondition = CurrentCondition;
    }
};
