#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * RiftVaultUI module — MVVM UI stack.
 *
 * Provides ViewModels, Widget base classes, Widget Controllers,
 * and the ViewModel subsystem. Does not ship Blueprint widget assets
 * — those are game-specific.
 *
 * Module lifetime is managed by Unreal's module system. StartupModule and
 * ShutdownModule are intentionally empty because all per-player state lives
 * in URiftInventoryUISubsystem (a LocalPlayerSubsystem), which Unreal
 * creates and destroys automatically alongside each local player.
 */
class FRiftVaultUIModule : public IModuleInterface
{
public:

    /** Called when the module is loaded into memory. No-op — subsystem handles per-player init. */
    virtual void StartupModule() override;

    /** Called before the module is unloaded. No-op — subsystem handles per-player teardown. */
    virtual void ShutdownModule() override;
};
