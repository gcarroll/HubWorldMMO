#include "RiftVaultCore.h"

// LOCTEXT_NAMESPACE scopes FText literals to this module so they don't
// collide with identically-keyed strings from other modules.
#define LOCTEXT_NAMESPACE "FRiftVaultCoreModule"

void FRiftVaultCoreModule::StartupModule()
{
    // No startup work needed. All Core types are statically initialized
    // (gameplay tags via UE_DEFINE_GAMEPLAY_TAG, data assets via the
    // asset manager). Nothing to construct here at module load time.
}

void FRiftVaultCoreModule::ShutdownModule()
{
    // No teardown needed. Core owns no heap resources that outlive UObject
    // garbage collection or require explicit shutdown ordering.
}

#undef LOCTEXT_NAMESPACE

// Registers FRiftVaultCoreModule with the Unreal Engine module system under
// the name "RiftVaultCore", which must match the ModuleName in RiftVault.uplugin.
IMPLEMENT_MODULE(FRiftVaultCoreModule, RiftVaultCore)
