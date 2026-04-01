#include "RiftVaultUI.h"

// LOCTEXT_NAMESPACE scopes FText::FromString literals to this module for localization.
// Required boilerplate even when no localized text is currently defined here.
#define LOCTEXT_NAMESPACE "FRiftVaultUIModule"

void FRiftVaultUIModule::StartupModule()
{
    // Intentionally empty.
    // All per-player state is managed by URiftInventoryUISubsystem, which Unreal
    // creates automatically for each local player. No global module-level state needed.
}

void FRiftVaultUIModule::ShutdownModule()
{
    // Intentionally empty.
    // URiftInventoryUISubsystem::Deinitialize() handles ViewModels cleanup when
    // the local player is torn down, so nothing needs to happen at module shutdown.
}

#undef LOCTEXT_NAMESPACE

// Registers FRiftVaultUIModule as the implementation for the "RiftVaultUI" module name.
// This macro must appear exactly once per module, in any .cpp file compiled into it.
IMPLEMENT_MODULE(FRiftVaultUIModule, RiftVaultUI)
