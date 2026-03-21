#pragma once

#include "CoreMinimal.h"
#include "EContainerType.generated.h"

/**
 * Categorizes the role of a container within the inventory system.
 *
 * Used by container definitions and acceptance rules to distinguish
 * what kind of container an item is being moved into or out of.
 * Equipment and Vendor containers apply additional validation rules
 * beyond simple capacity checks.
 */
UENUM(BlueprintType)
enum class EContainerType : uint8
{
    /** The player's main inventory — general item storage. */
    PlayerInventory,

    /** Equipment slots on the pawn. Items here drive GAS ability grants and Mutable mesh updates. */
    Equipment,

    /** A stash or shared storage location, typically world-persistent. */
    Stash,

    /** A vendor's stock. Read by the economy system for buy/sell transactions. */
    Vendor,

    /** A loot container in the world. Cleared after the player loots it. */
    Loot,

    /** Internal use only. */
    Undefined = UINT8_MAX UMETA(Hidden)
};
