#pragma once

#include "CoreMinimal.h"
#include "EItemLifecycle.generated.h"

/**
 * Lifecycle states for a RiftVault item instance.
 *
 * Items move through these states in order as they are accepted into an
 * inventory, made live, and eventually removed. Systems that react to item
 * changes (e.g. equipment grant logic, UI badges) should gate their work
 * on this value to avoid operating on partially initialized or already-dead
 * item instances.
 *
 * State transitions are managed exclusively by URiftInventoryComponent.
 * External code must not write this field directly.
 *
 * Typical flow:
 *   Initializing → Active → PendingRemoval → Removed
 */
UENUM(BlueprintType)
enum class EItemLifecycle : uint8
{
    /**
     * Item has been accepted by the inventory and is being initialized.
     * Fragments are being set up; the item must not be queried or mutated
     * by external systems during this phase.
     */
    Initializing,

    /**
     * All fragments have been initialized. The item is fully live inside
     * the inventory and may be freely queried, equipped, stacked, etc.
     */
    Active,

    /**
     * Item has been marked for removal and is awaiting cleanup.
     * Fragment teardown is in progress; treat as logically absent.
     */
    PendingRemoval,

    /**
     * Item has been fully removed from the inventory.
     * The URiftItemInstance object may still exist transiently in memory
     * (awaiting GC) but should never be read or operated on in this state.
     */
    Removed
};
