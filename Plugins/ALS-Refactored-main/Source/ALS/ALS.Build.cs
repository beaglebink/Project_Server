using UnrealBuildTool;

public class ALS : ModuleRules
{
	public ALS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;

		bEnableNonInlinedGenCppWarnings = true;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core", "CoreUObject", "Engine", "GameplayTags",
			"AnimGraphRuntime", "RigVM", "ControlRig", "UMG", "EnhancedInput"
			// FPSKitALSRefactored ”Ѕ–јЌќ Ч плагин не должен зависеть от игрового модул€
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"EngineSettings", "NetCore", "PhysicsCore", "Niagara"
		});

		if (Target.Type == TargetRules.TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(new[] { "MessageLog" });
		}
	}
}