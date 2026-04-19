using UnrealBuildTool;
using System.IO;
using System.Collections.Generic;

public class FPSKitALSRefactoredEditor : ModuleRules
{
    public FPSKitALSRefactoredEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore",
            "UnrealEd", "PropertyEditor", "Slate", "SlateCore", "EditorStyle", "AssetRegistry",
            // runtime-модуль, где определён ALocationAnchorActor
            "FPSKitALSRefactored"
        });

        // Явно добавляем include-пути к заголовкам runtime-модуля
        string RuntimeModuleBase = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "FPSKitALSRefactored"));
        PrivateIncludePaths.Add(Path.Combine(RuntimeModuleBase, "CoreGameplay", "LocationSystem"));
        // Так же добавляем Public на случай, если у вас есть публичные заголовки
        PrivateIncludePaths.Add(Path.Combine(RuntimeModuleBase, "Public"));

        PrivateIncludePaths.Add("FPSKitALSRefactoredEditor/Private");
    }
}