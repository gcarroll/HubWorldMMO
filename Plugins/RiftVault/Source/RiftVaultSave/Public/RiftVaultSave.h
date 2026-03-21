#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * RiftVaultSave module — persistence boundary.
 *
 * Owns IRiftPersistenceInterface, FRiftInventorySaveData, and all
 * serialization structs. Exists as a dedicated module so these types
 * can be safely referenced in UFUNCTION signatures in RiftVaultInventory
 * without cross-module UHT violations.
 */
class FRiftVaultSaveModule : public IModuleInterface
{
public:

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
