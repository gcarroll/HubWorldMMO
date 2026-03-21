using UnrealBuildTool;

public class RiftVaultUI : ModuleRules
{
    public RiftVaultUI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "GameplayAbilities",
            "GameplayTags",
            "ModelViewViewModel",
            "RiftVaultCore",
            "RiftVaultSave",
            "RiftVaultInventory",
            "UMG",
            "DeveloperSettings",
            "InputCore"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore"
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
