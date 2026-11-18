using UnrealBuildTool;

public class NetworkProjectEditorTarget : TargetRules
{
    public NetworkProjectEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("NetworkProject");
    }
}
