#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * RiftVaultLoot module — world pickup actors.
 *
 * Minimal stub that provides just enough for testing the Equipment module:
 *   ARiftPickup           — placeable/spawnable world pickup (overlap → AddItem)
 *   URiftPickupComponent  — composable component for any actor that should act as a pickup
 *
 * Depends on RiftVaultCore and RiftVaultInventory.
 */
class FRiftVaultLootModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
