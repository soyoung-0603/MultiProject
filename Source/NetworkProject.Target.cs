using UnrealBuildTool;

public class NetworkProjectTarget : TargetRules
{
    public NetworkProjectTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("NetworkProject");
    }
}
