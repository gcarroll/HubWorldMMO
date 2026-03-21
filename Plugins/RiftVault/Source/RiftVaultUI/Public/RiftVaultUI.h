#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * RiftVaultUI module — MVVM UI stack.
 *
 * Provides ViewModels, Widget base classes, Widget Controllers,
 * and the ViewModel subsystem. Does not ship Blueprint widget assets
 * — those are game-specific.
 */
class FRiftVaultUIModule : public IModuleInterface
{
public:

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
