#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * RiftVaultTests module — automated test specs for silent-failure-risk systems.
 *
 * Tests are written using DEFINE_SPEC and live in Private/Specs/.
 * This module is DeveloperTool type — it is never shipped with the game.
 *
 * Priority test areas (see TDD Section 15):
 *   High:   Stacking, Serialization, Crafting, Condition
 *   Medium: Economy, Loot
 *   Low:    Equipment ability grants/revokes
 */
class FRiftVaultTestsModule : public IModuleInterface
{
public:

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

};
