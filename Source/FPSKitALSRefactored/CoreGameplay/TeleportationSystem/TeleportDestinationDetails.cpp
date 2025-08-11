#include "TeleportDestinationDetails.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Editor.h"
#include "ScopedTransaction.h"
#include "TeleportDestination.h"
#include "SlotSceneComponent.h"
#include "Engine/Selection.h"
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "TeleportDestinationDetails"

TSharedRef<IDetailCustomization> FTeleportDestinationDetails::MakeInstance()
{
    return MakeShareable(new FTeleportDestinationDetails);
}

void FTeleportDestinationDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    CachedDetailBuilder = &DetailBuilder;

    TArray<TWeakObjectPtr<UObject>> Objects;
    DetailBuilder.GetObjectsBeingCustomized(Objects);

    if (Objects.Num() > 0 && Objects[0].IsValid())
    {
        EditedActor = Cast<ATeleportDestination>(Objects[0].Get());
    }

    IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Teleport Slots");

    Category.AddCustomRow(LOCTEXT("AddSlot", "Add Slot"))
        .ValueContent()
        [
            SNew(SButton)
                .Text(LOCTEXT("AddSlotButton", "Add Slot"))
                .OnClicked(FOnClicked::CreateSP(this, &FTeleportDestinationDetails::OnAddSlotClicked))
        ];
}

FReply FTeleportDestinationDetails::OnAddSlotClicked()
{
    if (!EditedActor.IsValid())
    {
        return FReply::Handled();
    }

    const FScopedTransaction Transaction(LOCTEXT("AddTeleportSlot", "Add Teleport Slot"));

    EditedActor->Modify();

    // Генерация уникального имени
    FName DesiredName = FName(TEXT("TeleportSlot"));
    FName UniqueName = MakeUniqueObjectName(
        EditedActor.Get(),
        USlotSceneComponent::StaticClass(),
        DesiredName
    );

    // Создание компонента
    USlotSceneComponent* NewSlot = NewObject<USlotSceneComponent>(
        EditedActor.Get(),
        USlotSceneComponent::StaticClass(),
        UniqueName,
        RF_Transactional
    );

    if (NewSlot)
    {
        NewSlot->SetupAttachment(EditedActor->GetRootComponent());
        NewSlot->RegisterComponent();
        EditedActor->AddInstanceComponent(NewSlot);
        NewSlot->OnComponentCreated();
        NewSlot->Rename(*UniqueName.ToString(), EditedActor.Get());

        // Выделение нового компонента в редакторе
        GEditor->SelectNone(false, true);
        GEditor->SelectComponent(NewSlot, true, true, true);
    }

    // Обновление Details Panel
    if (CachedDetailBuilder)
    {
        CachedDetailBuilder->ForceRefreshDetails();
    }

    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE