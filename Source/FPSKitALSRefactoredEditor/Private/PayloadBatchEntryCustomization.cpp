#include "PayloadBatchEntryCustomization.h"

#include "PayloadBatchEntry.h"
#include "OutcomePayload.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "IDetailPropertyRow.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IDetailCustomization> FPayloadBatchEntryCustomization::MakeInstance()
{
    return MakeShared<FPayloadBatchEntryCustomization>();
}

void FPayloadBatchEntryCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    UE_LOG(LogTemp, Warning, TEXT(">>> PayloadBatchEntryCustomization RUNS"));

    // 1) Скрываем свойство-держатель по handle (надёжнее, чем FName).
    TSharedRef<IPropertyHandle> PayloadHandle =
        DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UPayloadBatchEntry, Payload));
    if (PayloadHandle->IsValidHandle())
    {
        DetailBuilder.HideProperty(PayloadHandle);
    }
    else
    {
        // Fallback — если handle не нашёлся.
        DetailBuilder.HideProperty(FName("Payload"));
    }

    // 2) Собираем payload-объекты со всех выбранных entry.
    TArray<UObject*> PayloadObjects;
    for (const TWeakObjectPtr<UObject>& Obj : DetailBuilder.GetSelectedObjects())
    {
        if (UPayloadBatchEntry* Entry = Cast<UPayloadBatchEntry>(Obj.Get()))
        {
            if (Entry->Payload)
            {
                PayloadObjects.Add(Entry->Payload);
            }
        }
    }

    // 3) Категория "3. Data".
    IDetailCategoryBuilder& DataCat = DetailBuilder.EditCategory(
        "3. Data", FText::GetEmpty(), ECategoryPriority::Important);

    if (PayloadObjects.Num() > 0)
    {
        // 3.1) Строка с именем класса payload'а (серая, неактивная).
        const FString ClassName = PayloadObjects[0]->GetClass()->GetName();
        DataCat.AddCustomRow(FText::FromString(TEXT("Payload")))
            .NameContent()
            [
                SNew(STextBlock).Text(FText::FromString(TEXT("Payload")))
            ]
            .ValueContent()
            [
                SNew(STextBlock)
                    .Text(FText::FromString(ClassName))
                    .ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
            ];

        // 3.2) Поля payload'а — редактируются.
        FAddPropertyParams Params;
        Params.HideRootObjectNode(true);
        DataCat.AddExternalObjects(PayloadObjects, EPropertyLocation::Default, Params);
    }
    else
    {
        DataCat.AddCustomRow(FText::FromString(TEXT("Payload")))
            .WholeRowContent()
            [
                SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Payload отсутствует — выбери команду")))
            ];
    }
}