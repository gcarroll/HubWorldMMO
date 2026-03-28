#pragma once

#include "CoreMinimal.h"
#include "Data/URiftContainerDefinition.h"
#include "URiftContainerDefinition_Backpack.generated.h"

/**
 * Ready-made container definition for a player's backpack (general inventory).
 *
 * Pre-configured in its C++ constructor with:
 *   - ContainerTag  = Rift.Container.Backpack
 *   - GridWidth     = 5, GridHeight = 8  (40 slots — a typical MMO backpack size)
 *   - ContainerType = PlayerInventory
 *   - No item-compatibility filter (accepts all item types)
 *
 * Usage: pass GetDefault<URiftContainerDefinition_Backpack>() to
 * URiftInventoryComponent::AddContainer — no content-browser asset required.
 * Subclass in Blueprint if you want to override the grid size or add a filter.
 */
UCLASS(DisplayName = "Backpack Container Definition")
class RIFTVAULTCORE_API URiftContainerDefinition_Backpack : public URiftContainerDefinition
{
    GENERATED_BODY()

public:

    URiftContainerDefinition_Backpack();
};
