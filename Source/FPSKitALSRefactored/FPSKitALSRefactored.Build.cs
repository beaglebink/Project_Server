using UnrealBuildTool;
using System.IO;

public class FPSKitALSRefactored : ModuleRules
{
    public FPSKitALSRefactored(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine",
            "EnhancedInput",
            "UMG",
            "Niagara",
            "GameplayTags",
            "InputCore",
            "Slate",
            "SlateCore",
            "Json",
            "JsonUtilities",
        });

        // ALS и ALSExtras — только как приватные зависимости,
        // чтобы не создавать циклическую зависимость через ALSEditor
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "AppFramework",
            "ALS",       // приватная зависимость — не экспортируется во внешние модули
            "ALSExtras", // приватная зависимость
        });

        // Editor-only зависимости — строго только при сборке с редактором
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "EditorStyle",
                "PropertyEditor",
                "UnrealEd",
            });
        }

        // Корень модуля — для include "CoreGameplay/..."
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
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
    }
}
