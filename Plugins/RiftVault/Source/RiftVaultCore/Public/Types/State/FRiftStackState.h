#pragma once

#include "CoreMinimal.h"
#include "Types/State/FRiftFragmentState.h"
#include "FRiftStackState.generated.h"

/**
 * Per-instance state for URiftFragment_Stack.
 *
 * Tracks the current quantity of items in this stack. The maximum stack size
 * is defined on the fragment (type-level data) — this struct only holds the
 * runtime value that changes as items are added, removed, or split.
 */
USTRUCT(BlueprintType, DisplayName = "Stack State")
struct RIFTVAULTCORE_API FRiftStackState : public FRiftFragmentState
{
    GENERATED_BODY()

    /** Current number of items in this stack. Always >= 1. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stack State", meta = (UIMin = 1, ClampMin = 1))
    int32 CurrentQuantity = 1;

    /** Last quantity before replication. Used for change detection on clients. */
    UPROPERTY(NotReplicated)
    int32 LastQuantity = 0;

    virtual void SynchronizeReplicationState() override
    {
        LastQuantity = CurrentQuantity;
    }
};
