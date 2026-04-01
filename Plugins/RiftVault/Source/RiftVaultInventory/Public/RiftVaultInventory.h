#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * RiftVaultInventory module — the heart of the system.
 *
 * Contains item definitions, all fragment classes, item instances, containers,
 * the inventory component, the processing queue, replication, and persistence
 * interface calls.
 *
 * URiftInventoryComponent lives on APlayerState, not APawn, so inventory
 * survives pawn death and respawn. This is a permanent, closed decision.
 */
class FRiftVaultInventoryModule : public IModuleInterface
{
public:

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
