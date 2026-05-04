using UnrealBuildTool;
using System.IO;

public class FPSKitALSRefactored : ModuleRules
{
    public FPSKitALSRefactored(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        OptimizeCode = CodeOptimization.Never;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "ALS",
            "ALSExtras",
            "EnhancedInput",
            "UMG",
            "Niagara",
            "GameplayTags",
            "InputCore",
            "Slate",
            "SlateCore",
            "Json",
            "JsonUtilities"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "MoviePlayer",
            "Slate",
            "SlateCore"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "EditorStyle",
                "PropertyEditor",
                "UnrealEd",
                "MoviePlayer",
                "BlueprintGraph",
                "KismetCompiler",
                "GraphEditor"
            });
        }

        PublicIncludePaths.Add(ModuleDirectory);
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "CoreGameplay"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "CoreGameplay", "EventBusSystem"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "CoreGameplay", "InteractionSystem"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "CoreGameplay", "InteriorInstanceSystem"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "CoreGameplay", "SpawnGroupSystem"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "CoreGameplay", "WorldStateSystem"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "CoreGameplay", "TeleportationSystem"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "CoreGameplay", "Weapon"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "CoreGameplay", "Cubixon"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "CoreGameplay", "MissionSystem"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "CoreGameplay", "MissionSystem", "ProxySystem"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "CoreGameplay", "SaveGame"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "CoreGameplay", "LocationSystem"));
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
    }
}
