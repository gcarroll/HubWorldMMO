#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * RiftVaultCore module — the foundation layer of the RiftVault plugin.
 *
 * This module is intentionally kept dependency-free with respect to gameplay
 * systems. It contains only:
 *   - Pure data types (enums, structs)
 *   - Item and container state structs (FRiftFragmentState and subclasses)
 *   - Gameplay tag declarations (RiftVaultTags.h)
 *   - Lightweight interfaces (IRiftInventoryInterface, IRiftEquipmentInterface)
 *   - The container definition data asset (URiftContainerDefinition)
 *
 * Higher-level modules (RiftVaultInventory, RiftVaultEquipment, etc.) all
 * depend on this module. This module must never depend on them — that would
 * create a circular dependency.
 *
 * No gameplay logic, no components, no managers live here.
 */
class FRiftVaultCoreModule : public IModuleInterface
{
public:

    /**
     * Called by the engine when the module is loaded into memory.
     * Currently a no-op — all Core initialization is data-driven and requires
     * no explicit startup code.
     */
    virtual void StartupModule() override;

    /**
     * Called by the engine when the module is about to be unloaded.
     * Currently a no-op — Core holds no resources that need manual teardown.
     */
    virtual void ShutdownModule() override;

};
