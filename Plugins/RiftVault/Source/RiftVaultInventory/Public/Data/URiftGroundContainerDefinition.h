#pragma once

#include "CoreMinimal.h"
#include "Data/URiftContainerDefinition.h"
#include "URiftGroundContainerDefinition.generated.h"

/**
 * Ready-made container definition for ground-drop pickups.
 *
 * Pre-configured in its C++ constructor with:
 *   - ContainerTag  = Rift.Container.Ground
 *   - Capacity      = 10x10 = 100 slots (accepts any number of dropped item stacks)
 *   - No item-compatibility filter (accepts all item types)
 *
 * Used by ARiftPickup::DropInventory.
 */
UCLASS(DisplayName = "Ground Container Definition")
class RIFTVAULTINVENTORY_API URiftGroundContainerDefinition : public URiftContainerDefinition
{
    GENERATED_BODY()

public:

    URiftGroundContainerDefinition();
};
