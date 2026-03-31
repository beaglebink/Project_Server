using UnrealBuildTool;

public class ALSExtras : ModuleRules
{
    public ALSExtras(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "AIModule",
            "ALS", "UMG", "GameplayTags", "Niagara",
            // FPSKitALSRefactored stays here - ALSExtras needs full linking
            // with types like UInteractiveItemComponent, UInteractivePickerComponent
            // Cycle is broken: FPSKitALSRefactored has only PRIVATE dep on ALSExtras
            // so ALSExtras is not exposed to consumers of FPSKitALSRefactored
            // (ѕолна€ зависимость нужна дл€ линковки с типами FPSKitALSRefactored)
            // (÷икл разорван: FPSKitALSRefactored имеет только PRIVATE зависимость)
            "FPSKitALSRefactored"
        });

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "EnhancedInput", "ALSCamera", "Slate", "SlateCore", "InputCore"
        });
    }
}