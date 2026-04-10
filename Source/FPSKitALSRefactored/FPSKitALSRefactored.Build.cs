using System.IO;
using UnrealBuildTool;

public class FPSKitALSRefactored : ModuleRules
{
    public FPSKitALSRefactored(ReadOnlyTargetRules Target) : base(Target)
    {
        OptimizeCode = CodeOptimization.Never;

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore",
            "GameplayTags", "UMG", "Niagara", "Json", "JsonUtilities", "EnhancedInput", "ALS", "ALSExtras"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "Slate", "SlateCore", "ApplicationCore"
        });

        if (Target.Type == TargetType.Editor)
        {
            PublicDependencyModuleNames.AddRange(new string[] { "UnrealEd", "Blutility", "AssetRegistry" });
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
