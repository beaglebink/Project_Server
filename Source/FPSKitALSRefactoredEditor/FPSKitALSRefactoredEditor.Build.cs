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
            "FPSKitALSRefactored"
        });

        string RuntimeModuleBase = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "FPSKitALSRefactored"));
        PrivateIncludePaths.Add(Path.Combine(RuntimeModuleBase, "CoreGameplay", "LocationSystem"));
        PrivateIncludePaths.Add(Path.Combine(RuntimeModuleBase, "Public"));

        PrivateIncludePaths.Add("FPSKitALSRefactoredEditor/Private");
    }
}