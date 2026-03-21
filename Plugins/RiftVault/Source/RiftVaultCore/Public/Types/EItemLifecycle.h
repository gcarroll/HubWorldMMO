#pragma once

#include "CoreMinimal.h"
#include "EItemLifecycle.generated.h"

/**
 * Lifecycle states for a RiftVault item instance.
 *
 * Items move through these states in order as they are added to an inventory,
 * made active, and eventually removed. Systems that need to react to item
 * state changes (e.g. equipment, UI) can gate their logic on this value.
 */
UENUM(BlueprintType)
enum class EItemLifecycle : uint8
{
    /** Item has been accepted by the inventory and is being initialized. */
    Initializing,

    /** All fragments have been initialized. Item is live in the inventory. */
    Active,

    /** Item has been marked for removal and is awaiting cleanup. */
    PendingRemoval,

    /** Item has been fully removed from the inventory. */
    Removed
};
