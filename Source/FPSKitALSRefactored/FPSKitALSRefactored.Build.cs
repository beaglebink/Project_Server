using System.IO;
using UnrealBuildTool;

public class FPSKitALSRefactored : ModuleRules
{
    public FPSKitALSRefactored(ReadOnlyTargetRules Target) : base(Target)
    {
        OptimizeCode = CodeOptimization.Never;

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "AssetRegistry",
            "ALS", "ALSCamera", "EnhancedInput", "UMG",
            "Niagara", "NiagaraCore", "NiagaraShader",
            "Json", "JsonUtilities"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "Slate", "SlateCore", "ApplicationCore",
            "ALSExtras"
        });

        if (Target.Type == TargetType.Editor)
        {
            PublicDependencyModuleNames.Add("UnrealEd");
        }

        PublicIncludePaths.AddRange(new string[] {
            "FPSKitALSRefactored/CoreGameplay/EventBusSystem",
            "FPSKitALSRefactored/CoreGameplay/ActorStateSystem",
            "FPSKitALSRefactored/CoreGameplay/SpawnGroupSystem"
        });

        PublicIncludePaths.AddRange(new string[] {
            Path.Combine(ModuleDirectory, "Public"),
            Path.Combine(ModuleDirectory, "CoreGameplay", "Cubixon")
        });
    }
}
