using UnrealBuildTool;

public class RiftVaultEquipment : ModuleRules
{
    public RiftVaultEquipment(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "GameplayAbilities",
            "GameplayTags",
            "GameplayTasks",
            "NetCore",
            "RiftVaultCore",
            "RiftVaultInventory"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "CoreUObject",
            "Engine",
            "CustomizableObject"  // UCustomizableSkeletalComponent, UCustomizableObjectInstance
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
