using UnrealBuildTool;

public class ALS : ModuleRules
{
    public ALS(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "UMG",
            "EnhancedInput",
            "Niagara",
            "PhysicsCore",
            "NetCore",
            "EngineSettings"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "AIModule",
            "GameplayTasks",
            "GameplayTags",
            "AnimGraphRuntime",
            "AnimationCore",
            "ControlRig",
            "RigVM"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "MessageLog",
                "UnrealEd"
            });
        }
    }
}