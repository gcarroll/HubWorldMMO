#pragma once

#include "CoreMinimal.h"
#include "EContainerType.generated.h"

/**
 * Categorizes the role of a container within the RiftVault inventory system.
 *
 * Used by URiftContainerDefinition and acceptance rule logic to distinguish
 * what kind of container an item is being moved into or out of. Different
 * container types apply different validation layers:
 *
 *   PlayerInventory — capacity check + ItemCompatibilityQuery
 *   Equipment       — capacity check + slot-tag matching + equippable trait required
 *   Stash           — capacity check + ItemCompatibilityQuery (world-persistent)
 *   Vendor          — capacity check; economy system applies buy/sell price rules
 *   Loot            — capacity check; cleared after the player finishes looting
 *   Undefined       — sentinel value; never set by designer-authored assets
 */
UENUM(BlueprintType)
enum class EContainerType : uint8
{
    /** The player's main inventory — general-purpose item storage. */
    PlayerInventory,

    /**
     * Equipment slots on the owning pawn.
     * Items placed here are considered "equipped" and drive:
     *   - GAS ability grants (via URiftFragment_Equippable)
     *   - Mutable skeletal mesh parameter updates (via URiftMutableEquipmentComponent)
     */
    Equipment,

    /**
     * A stash or shared storage location, typically world-persistent.
     * Stashes are saved independently of the player's backpack and may
     * be shared across play sessions or between players.
     */
    Stash,

    /**
     * A vendor's stock container.
     * Read by the economy system to populate buy/sell UI and validate
     * currency availability before completing transactions.
     */
    Vendor,

    /**
     * A loot container in the world (e.g. chest, dropped item pile).
     * Managed by the loot system; cleared after the player collects items.
     */
    Loot,

    /**
     * Sentinel / uninitialized value.
     * Hidden from the editor — used internally to detect unset ContainerType
     * fields during data validation. Designer-authored assets must never use this.
     */
    Undefined = UINT8_MAX UMETA(Hidden)
};
