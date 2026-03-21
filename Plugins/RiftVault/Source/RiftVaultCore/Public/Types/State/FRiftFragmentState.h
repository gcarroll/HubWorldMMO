#pragma once

#include "CoreMinimal.h"
#include "FRiftFragmentState.generated.h"

/**
 * Base struct for all RiftVault Item States.
 *
 * Item States store per-instance runtime data for fragments that need it.
 * The definition (URiftItemFragment) holds type-level data shared across all
 * instances of an item. The state holds instance-level data unique to one
 * specific item in the inventory (e.g. this sword's current condition is 73).
 *
 * Not all fragments need a state. URiftFragment_Display has no state — the
 * name and icon are the same for every instance of that item type. Only
 * fragments that track mutable per-instance data define a state subclass.
 *
 * Naming convention: FRift<Capability>State (e.g. FRiftStackState, FRiftConditionState).
 *
 * States are stored via TInstancedStruct on URiftItemInstance and replicated
 * using FFastArraySerializer. SynchronizeReplicationState() is called by the
 * replication system after broadcasting an update — use it to sync any
 * "previous value" tracking fields (e.g. LastStackSize = CurrentStackSize).
 */
USTRUCT(BlueprintType)
struct RIFTVAULTCORE_API FRiftFragmentState
{
    GENERATED_BODY()

    FRiftFragmentState() = default;
    virtual ~FRiftFragmentState() noexcept = default;

    /**
     * Called by the Fast Array replication system after an update has been
     * broadcast to owning clients. Use this to synchronize any fields that
     * track previous values for change detection.
     */
    virtual void SynchronizeReplicationState() { }
};
