using UnrealBuildTool;

public class NetworkProject : ModuleRules
{
    public NetworkProject(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "DeveloperSettings", "HTTP" });
        PrivateDependencyModuleNames.Add("Json");
    }
}
