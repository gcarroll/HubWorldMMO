#pragma once

#include "CoreMinimal.h"
#include "FRiftFragmentState.generated.h"

/**
 * Base struct for all RiftVault per-instance item state objects.
 *
 * DESIGN OVERVIEW
 * ---------------
 * Item definitions (URiftItemDefinition / URiftItemFragment) hold type-level
 * data that is identical for every instance of an item — things like the
 * display name, icon, max stack size, and ability sets to grant.
 *
 * Item states (this struct and its subclasses) hold instance-level data that
 * is unique to one specific item sitting in a specific inventory slot — things
 * like "this sword currently has 73 out of 100 condition" or "this stack
 * currently has 12 arrows in it."
 *
 * Not all fragments need a state. URiftFragment_Display has no state because
 * every instance of "Iron Sword" has the same name and icon. Only fragments
 * that track mutable per-instance values define a state subclass.
 *
 * NAMING CONVENTION
 * -----------------
 * FRift<Capability>State
 * Examples: FRiftStackState, FRiftConditionState
 *
 * STORAGE AND REPLICATION
 * -----------------------
 * States are stored via TInstancedStruct<FRiftFragmentState> on URiftItemInstance
 * and replicated using FFastArraySerializer. Because fast arrays deliver
 * individual element change notifications, the replication system calls
 * SynchronizeReplicationState() after broadcasting an update to clients.
 *
 * Subclasses that need to detect changes (e.g. "did the stack size decrease?")
 * should maintain a "Last*" shadow field and update it inside
 * SynchronizeReplicationState() so the old value is available during the
 * PostReplicatedChange callback before the sync call overwrites it.
 */
USTRUCT(BlueprintType)
struct RIFTVAULTCORE_API FRiftFragmentState
{
    GENERATED_BODY()

    FRiftFragmentState() = default;

    // Virtual destructor is required because subclasses are destroyed through
    // base-class pointers when TInstancedStruct releases them.
    virtual ~FRiftFragmentState() noexcept = default;

    /**
     * Called by the Fast Array replication system after a replicated update
     * has been broadcast to owning clients.
     *
     * Override this in subclasses to synchronize any "previous value" shadow
     * fields used for change detection. For example, FRiftStackState copies
     * CurrentQuantity into LastQuantity here so that the PostReplicatedChange
     * callback can compare old vs. new before this sync occurs.
     *
     * The base implementation is intentionally empty — not all states need
     * change tracking, and virtuality is kept cheap via final overrides.
     */
    virtual void SynchronizeReplicationState() { }
};
