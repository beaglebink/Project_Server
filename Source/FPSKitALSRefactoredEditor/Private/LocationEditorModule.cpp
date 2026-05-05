#include "LocationEditorModule.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "LocationAnchorActorDetails.h"
#include "ProxyActorNamePinFactory.h"
#include "EdGraphUtilities.h"

#define LOCTEXT_NAMESPACE "FLocationEditorModule"

void FLocationEditorModule::StartupModule()
{
    if (!FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    }

    FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

    PropertyModule.RegisterCustomClassLayout(
        FName(TEXT("LocationAnchorActor")),
        FOnGetDetailCustomizationInstance::CreateStatic(&FLocationAnchorActorDetails::MakeInstance)
    );

    PropertyModule.NotifyCustomizationModuleChanged();

    // Регистрация Pin Factory для комбобокса ActorName
    TSharedPtr<FProxyActorNamePinFactory> PinFactory = MakeShareable(new FProxyActorNamePinFactory());
    FEdGraphUtilities::RegisterVisualPinFactory(PinFactory);
}

void FLocationEditorModule::ShutdownModule()
{
    if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
    {
        FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyModule.UnregisterCustomClassLayout(FName(TEXT("LocationAnchorActor")));
        PropertyModule.NotifyCustomizationModuleChanged();
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLocationEditorModule, FPSKitALSRefactoredEditor)