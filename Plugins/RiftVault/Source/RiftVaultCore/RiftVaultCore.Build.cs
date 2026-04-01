using UnrealBuildTool;

public class RiftVaultCore : ModuleRules
{
    public RiftVaultCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "GameplayAbilities",
            "GameplayTags",
            "GameplayTasks",
            "NetCore"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "CoreUObject",
            "DeveloperSettings",
            "Engine",
            "Niagara",
            "UMG",
            "Slate",
            "SlateCore"
        });

        AddVersionBasedDependencies(Target);
    }

    /**
     * Adds dependencies that require certain engine versions.
     * StructUtils was a separate module pre-5.5, built into the engine from 5.5 onwards.
     */
    public void AddVersionBasedDependencies(ReadOnlyTargetRules Target)
    {
        if (Target.Version is { MajorVersion: 5, MinorVersion: < 5 })
        {
            PublicDependencyModuleNames.Add("StructUtils");
        }
    }
}
