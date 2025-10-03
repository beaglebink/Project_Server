using UnrealBuildTool;

public class ALSExtras : ModuleRules
{
	public ALSExtras(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;

		bEnableNonInlinedGenCppWarnings = true;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core", "CoreUObject", "Engine", "AIModule", "ALS", "UMG", "GameplayTags", "FPSKitALSRefactored", "Niagara"
        });

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"EnhancedInput", "ALSCamera", "Slate", "SlateCore", "InputCore"
		});
	}
}