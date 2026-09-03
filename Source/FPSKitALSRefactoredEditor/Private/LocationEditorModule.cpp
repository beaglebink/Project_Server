#include "LocationEditorModule.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "LocationAnchorActorDetails.h"
#include "ChoreSystem/ChoreHistoryConditionAssetDetails.h"
#include "ChoreSystem/MissionConditionAssetDetails.h"
#include "ChoreSystem/MissionActiveConditionAssetDetails.h"   // <-- добавлено
#include "ChoreHistoryConditionAsset.h"
#include "MissionConditionAsset.h"
#include "MissionActiveConditionAsset.h"                     // <-- добавлено

#define LOCTEXT_NAMESPACE "FLocationEditorModule"

void FLocationEditorModule::StartupModule()
{
    if (!FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    }

    FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

    // Регистрация для ALocationAnchorActor
    PropertyModule.RegisterCustomClassLayout(
        FName(TEXT("LocationAnchorActor")),
        FOnGetDetailCustomizationInstance::CreateStatic(&FLocationAnchorActorDetails::MakeInstance)
    );

    // Регистрация для UChoreHistoryConditionAsset
    PropertyModule.RegisterCustomClassLayout(
        UChoreHistoryConditionAsset::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FChoreHistoryConditionAssetDetails::MakeInstance)
    );

    // Регистрация для UMissionConditionAsset
    PropertyModule.RegisterCustomClassLayout(
        UMissionConditionAsset::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FMissionConditionAssetDetails::MakeInstance)
    );

    // Регистрация для UMissionActiveConditionAsset
    PropertyModule.RegisterCustomClassLayout(
        UMissionActiveConditionAsset::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateStatic(&FMissionActiveConditionAssetDetails::MakeInstance)
    );

    PropertyModule.NotifyCustomizationModuleChanged();
}

void FLocationEditorModule::ShutdownModule()
{
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

        PropertyModule.UnregisterCustomClassLayout(FName(TEXT("LocationAnchorActor")));
        PropertyModule.UnregisterCustomClassLayout(UChoreHistoryConditionAsset::StaticClass()->GetFName());
        PropertyModule.UnregisterCustomClassLayout(UMissionConditionAsset::StaticClass()->GetFName());
        PropertyModule.UnregisterCustomClassLayout(UMissionActiveConditionAsset::StaticClass()->GetFName());

        PropertyModule.NotifyCustomizationModuleChanged();
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLocationEditorModule, FPSKitALSRefactoredEditor)