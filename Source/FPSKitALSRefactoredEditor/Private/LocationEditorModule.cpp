// LocationEditorModule.cpp
#include "LocationEditorModule.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "LocationAnchorActorDetails.h"
#include "ChoreSystem/ChoreHistoryConditionAssetDetails.h"
#include "ChoreSystem/MissionConditionAssetDetails.h"
#include "ChoreSystem/MissionActiveConditionAssetDetails.h"
#include "ChoreSystem/WorldStateConditionAssetDetails.h"
#include "ChoreHistoryConditionAsset.h"
#include "MissionConditionAsset.h"
#include "MissionActiveConditionAsset.h"
#include "WorldStateConditionAsset.h"
#include "ChoreSystem/TerminalGameConditionAssetDetails.h"
#include "TerminalGameConditionAsset.h"
#include "ChoreSystem/ChoreTimeSinceConditionAssetDetails.h"
#include "ChoreSystem/LocationConditionAssetDetails.h"
#include "LocationConditionAsset.h"
#include "ChoreSystem/TimeWindowConditionAssetDetails.h"
#include "ConditionStateSubsystem/TimeWindowConditionAsset.h"

#include "PayloadBatchEntry.h"
#include "PayloadBatchEntryCustomization.h"

#include "SceneActorRefCustomization.h"
#include "SceneActorRef.h"

#include "ChoreSystem/ScenarioConditionAssetDetails.h"
#include "ScenarioConditionAsset.h"

#define LOCTEXT_NAMESPACE "FLocationEditorModule"

void FLocationEditorModule::StartupModule()
{
    if (!FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    }

    FPropertyEditorModule& PropertyModule =
        FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

    PropertyModule.RegisterCustomClassLayout(
        FName(TEXT("LocationAnchorActor")),
        FOnGetDetailCustomizationInstance::CreateStatic(&FLocationAnchorActorDetails::MakeInstance));

    PropertyModule.RegisterCustomClassLayout(
        UChoreHistoryConditionAsset::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FChoreHistoryConditionAssetDetails::MakeInstance));

    PropertyModule.RegisterCustomClassLayout(
        UMissionConditionAsset::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FMissionConditionAssetDetails::MakeInstance));

    PropertyModule.RegisterCustomClassLayout(
        UMissionActiveConditionAsset::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FMissionActiveConditionAssetDetails::MakeInstance));

    PropertyModule.RegisterCustomClassLayout(
        UWorldStateConditionAsset::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FWorldStateConditionAssetDetails::MakeInstance));

    PropertyModule.RegisterCustomClassLayout(
        UTerminalGameConditionAsset::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FTerminalGameConditionAssetDetails::MakeInstance));

    PropertyModule.RegisterCustomClassLayout(
        ULocationConditionAsset::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FLocationConditionAssetDetails::MakeInstance));

    PropertyModule.RegisterCustomClassLayout(
        UTimeWindowConditionAsset::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FTimeWindowConditionAssetDetails::MakeInstance));

    // Регистрация для entry — оставляем на будущее, даже если UE её не применит.
    PropertyModule.RegisterCustomClassLayout(
        UPayloadBatchEntry::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FPayloadBatchEntryCustomization::MakeInstance));

    PropertyModule.RegisterCustomPropertyTypeLayout(
        FSceneActorRef::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FSceneActorRefCustomization::MakeInstance));

    PropertyModule.RegisterCustomClassLayout(
        UScenarioConditionAsset::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FScenarioConditionAssetDetails::MakeInstance)
    );

    PropertyModule.NotifyCustomizationModuleChanged();
}

void FLocationEditorModule::ShutdownModule()
{
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FPropertyEditorModule& PropertyModule =
            FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

        PropertyModule.UnregisterCustomClassLayout(FName(TEXT("LocationAnchorActor")));
        PropertyModule.UnregisterCustomClassLayout(UChoreHistoryConditionAsset::StaticClass()->GetFName());
        PropertyModule.UnregisterCustomClassLayout(UMissionConditionAsset::StaticClass()->GetFName());
        PropertyModule.UnregisterCustomClassLayout(UMissionActiveConditionAsset::StaticClass()->GetFName());
        PropertyModule.UnregisterCustomClassLayout(UWorldStateConditionAsset::StaticClass()->GetFName());
        PropertyModule.UnregisterCustomClassLayout(UTerminalGameConditionAsset::StaticClass()->GetFName());
        PropertyModule.UnregisterCustomClassLayout(ULocationConditionAsset::StaticClass()->GetFName());
        PropertyModule.UnregisterCustomClassLayout(UTimeWindowConditionAsset::StaticClass()->GetFName());
        PropertyModule.UnregisterCustomClassLayout(UPayloadBatchEntry::StaticClass()->GetFName());
        PropertyModule.UnregisterCustomPropertyTypeLayout(FSceneActorRef::StaticStruct()->GetFName());
        PropertyModule.UnregisterCustomClassLayout(UScenarioConditionAsset::StaticClass()->GetFName());

        PropertyModule.NotifyCustomizationModuleChanged();
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLocationEditorModule, FPSKitALSRefactoredEditor)