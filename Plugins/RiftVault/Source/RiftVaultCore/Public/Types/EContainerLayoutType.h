#pragma once

#include "CoreMinimal.h"
#include "EContainerLayoutType.generated.h"

/**
 * Controls how a container presents its slots and how the slot array is managed.
 *
 *   Grid — slots have a fixed visual position (row + column derived from slot index and
 *           ColumnCount). Empty slots are preserved as gaps so slot identity is stable.
 *           Used for backpacks, stashes, loot containers.
 *
 *   List — slots are compact and ordered. No empty gaps — removing an item collapses
 *           the array. Used for equipment containers, vendor stock, and any container
 *           where visual position is not meaningful.
 *
 * This value lives on URiftContainerDefinition and is read by both URiftContainer
 * (replication / slot management) and URiftContainerWidget (UI layout).
 */
UENUM(BlueprintType)
enum class EContainerLayoutType : uint8
{
    /** Grid layout — ColumnCount columns, rows derived automatically from slot count. */
    Grid,

    /** List layout — slots stacked linearly, no row/column positioning. */
    List
};
