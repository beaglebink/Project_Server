using System.IO;
using UnrealBuildTool;

public class FPSKitALSRefactored : ModuleRules
{
    public FPSKitALSRefactored(ReadOnlyTargetRules Target) : base(Target)
    {
		OptimizeCode = CodeOptimization.Never;
		
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "AssetRegistry", "ALS", "ALSCamera", "EnhancedInput", "UMG", "Niagara", "NiagaraCore", "NiagaraShader", "UnrealEd" });
        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "ApplicationCore", "ALSExtras" });
        /*
        PublicIncludePaths.AddRange(new string[] {
            "FPSKitALSRefactored/Public",
            Path.Combine(ModuleDirectory, "../../Plugins/ALS-Refactored-main/Source/ALSCamera/Public")
        });
        */
        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
