using UnrealBuildTool;

public class RiftVaultTests : ModuleRules
{
    public RiftVaultTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "GameplayAbilities",
            "GameplayTags",
            "GameplayTasks",
            "RiftVaultCore",
            "RiftVaultSave",
            "RiftVaultInventory",
            "RiftVaultEquipment",
            "RiftVaultLoot",
            "RiftVaultUI"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "CoreUObject",
            "Engine"
        });
    }
}
