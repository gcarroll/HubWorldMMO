#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * RiftVaultCore module — the foundation layer.
 * Contains all pure data types, enums, interfaces, tags, and item state structs.
 * No knowledge of managers, components, or gameplay systems beyond GAS tags.
 */
class FRiftVaultCoreModule : public IModuleInterface
{
public:

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

};
