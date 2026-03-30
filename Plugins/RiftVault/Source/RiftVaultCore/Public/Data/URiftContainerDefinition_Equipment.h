#pragma once

#include "CoreMinimal.h"
#include "Data/URiftContainerDefinition.h"
#include "URiftContainerDefinition_Equipment.generated.h"

/**
 * Ready-made container definition for a player's equipment container.
 *
 * Pre-configured in its C++ constructor with:
 *   - ContainerTag  = Rift.Container.Equipment
 *   - LayoutType = List, SlotCount = 12  (12 slots — one per equipment slot)
 *   - ContainerType = Equipment
 *   - Item filter: only items carrying the Rift.Item.Trait.Equippable tag are accepted
 *
 * Usage: pass GetDefault<URiftContainerDefinition_Equipment>() to
 * URiftInventoryComponent::AddContainer — no content-browser asset required.
 * Subclass in Blueprint to override slot count or add additional filters.
 */
UCLASS(DisplayName = "Equipment Container Definition")
class RIFTVAULTCORE_API URiftContainerDefinition_Equipment : public URiftContainerDefinition
{
    GENERATED_BODY()

public:

    URiftContainerDefinition_Equipment();
};
