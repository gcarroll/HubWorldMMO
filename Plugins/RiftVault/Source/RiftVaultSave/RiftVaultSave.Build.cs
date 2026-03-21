using UnrealBuildTool;

public class RiftVaultSave : ModuleRules
{
    public RiftVaultSave(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "GameplayTags",
            "NetCore",
            "RiftVaultCore"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "CoreUObject",
            "Engine"
        });

        AddVersionBasedDependencies(Target);
    }

    public void AddVersionBasedDependencies(ReadOnlyTargetRules Target)
    {
        if (Target.Version is { MajorVersion: 5, MinorVersion: < 5 })
        {
            PublicDependencyModuleNames.Add("StructUtils");
        }
    }
}
