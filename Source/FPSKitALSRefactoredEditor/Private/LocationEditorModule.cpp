#include "LocationEditorModule.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "LocationAnchorActorDetails.h"

#define LOCTEXT_NAMESPACE "FLocationEditorModule"

void FLocationEditorModule::StartupModule()
{
    if (!FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    }

    FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

    // ИСПРАВЛЕНО: имя класса без префикса "A"
    PropertyModule.RegisterCustomClassLayout(
        FName(TEXT("LocationAnchorActor")),
        FOnGetDetailCustomizationInstance::CreateStatic(&FLocationAnchorActorDetails::MakeInstance)
    );

    PropertyModule.NotifyCustomizationModuleChanged();
}

void FLocationEditorModule::ShutdownModule()
{
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
        // ИСПРАВЛЕНО: то же имя без префикса
        PropertyModule.UnregisterCustomClassLayout(FName(TEXT("LocationAnchorActor")));
        PropertyModule.NotifyCustomizationModuleChanged();
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLocationEditorModule, FPSKitALSRefactoredEditor)