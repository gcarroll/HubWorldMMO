#pragma once

#include "CoreMinimal.h"
#include "Types/State/FRiftFragmentState.h"
#include "FRiftStackState.generated.h"

/**
 * Per-instance state for URiftFragment_Stack.
 *
 * Tracks the current quantity of items in this stack. The maximum stack size
 * is defined on URiftFragment_Stack itself (type-level, shared across all
 * instances of that item type). This struct holds only the runtime value that
 * changes as items are added, removed, split, or merged.
 *
 * IMPORTANT — Minimum quantity is 1, not 0.
 * RemoveQuantity floors at 1 rather than allowing the stack to reach zero.
 * A zero-quantity stack is invalid; the inventory removes the item instance
 * entirely before quantity would hit zero. See RemoveQuantity implementation
 * for details.
 *
 * REPLICATION
 * -----------
 * CurrentQuantity is replicated as part of the owning FFastArraySerializer
 * entry on URiftInventoryComponent. LastQuantity is NotReplicated — it exists
 * purely as a client-side shadow so PostReplicatedChange callbacks can compute
 * the delta (e.g. "player gained 3 arrows").
 */
USTRUCT(BlueprintType, DisplayName = "Stack State")
struct RIFTVAULTCORE_API FRiftStackState : public FRiftFragmentState
{
    GENERATED_BODY()

    /**
     * Current number of items in this stack.
     *
     * Always >= 1. The inventory removes the item instance before quantity
     * would reach zero. UIMin/ClampMin prevent accidental zeroing in the editor.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stack State", meta = (UIMin = 1, ClampMin = 1))
    int32 CurrentQuantity = 1;

    /**
     * Shadow copy of CurrentQuantity from before the most recent replication update.
     *
     * Not replicated — lives only on the client. Set by SynchronizeReplicationState()
     * after a replicated change has been broadcast. Use this in PostReplicatedChange
     * to compute how much the stack size changed without needing to cache it elsewhere.
     *
     * Example:
     *   int32 Delta = NewState.CurrentQuantity - NewState.LastQuantity;
     */
    UPROPERTY(NotReplicated)
    int32 LastQuantity = 0;

    /**
     * Synchronizes LastQuantity with CurrentQuantity after a replication update.
     *
     * Called by the Fast Array replication system on clients once the change
     * notification has been dispatched. After this call, LastQuantity == CurrentQuantity,
     * so any delta computation must happen in the PostReplicatedChange callback
     * BEFORE SynchronizeReplicationState() is invoked.
     */
    virtual void SynchronizeReplicationState() override
    {
        LastQuantity = CurrentQuantity;
    }
};
