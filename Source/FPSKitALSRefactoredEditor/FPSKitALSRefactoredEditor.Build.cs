using UnrealBuildTool;
using System.IO;
using System.Collections.Generic;

public class FPSKitALSRefactoredEditor : ModuleRules
{
    public FPSKitALSRefactoredEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        OptimizeCode = CodeOptimization.Never;

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore",
            "UnrealEd", "PropertyEditor", "Slate", "SlateCore", "AssetRegistry",
            "FPSKitALSRefactored",
            "EditorSubsystem"
        });

        string RuntimeModuleBase = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "FPSKitALSRefactored"));

        // Пути к папкам рантайм-модуля, из которых editor-модуль включает хедеры
        PrivateIncludePaths.Add(Path.Combine(RuntimeModuleBase, "Public"));
        PrivateIncludePaths.Add(Path.Combine(RuntimeModuleBase, "CoreGameplay", "LocationSystem"));
        PrivateIncludePaths.Add(Path.Combine(RuntimeModuleBase, "CoreGameplay", "ChoreSystem"));
        PrivateIncludePaths.Add(Path.Combine(RuntimeModuleBase, "CoreGameplay", "ScenarioManagerSubsystem")); // ← для PayloadBatchEntry.h

        PrivateIncludePaths.Add("FPSKitALSRefactoredEditor/Private");
    }
}