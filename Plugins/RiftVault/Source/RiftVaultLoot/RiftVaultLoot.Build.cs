using UnrealBuildTool;

public class RiftVaultLoot : ModuleRules
{
    public RiftVaultLoot(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "GameplayTags",
            "RiftVaultCore",
            "RiftVaultInventory"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "CoreUObject",
            "Engine"
        });
    }
}
