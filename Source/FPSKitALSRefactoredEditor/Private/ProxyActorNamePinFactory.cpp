#include "ProxyActorNamePinFactory.h"
#include "ProxyManager.h"
#include "ActorProxy.h"
#include "FloorAsset.h"
#include "SNameComboBox.h"
#include "SGraphPin.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "K2Node_GetActorByClass.h"
#include "ScopedTransaction.h"

class SGraphPinProxyActorName : public SGraphPin, public FTSTickerObjectBase
{
    TArray<TSharedPtr<FName>> NameOptions;
    TSharedPtr<FName> CurrentSelection;
    TWeakObjectPtr<UEdGraphNode> CachedNode;
    FString CachedFloorAssetPath;
    FString CachedActorClassName;

public:
    SLATE_BEGIN_ARGS(SGraphPinProxyActorName) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, UEdGraphPin* InPin)
    {
        GraphPinObj = InPin;
        SGraphPin::Construct(SGraphPin::FArguments(), InPin);

        if (GraphPinObj && GraphPinObj->GetOwningNode())
        {
            CachedNode = GraphPinObj->GetOwningNode();
        }
    }

    // Тикаем и проверяем, не изменились ли входные пины
    virtual bool Tick(float DeltaTime) override
    {
        if (!CachedNode.IsValid() || !GraphPinObj)
            return true;

        UEdGraphNode* Node = CachedNode.Get();

        // Проверяем FloorAsset
        UEdGraphPin* FloorPin = Node->FindPin(TEXT("FloorAsset"));
        FString CurrentFloorPath = FloorPin && FloorPin->DefaultObject
            ? FloorPin->DefaultObject->GetPathName() : FString();

        // Проверяем ActorClass
        UEdGraphPin* ClassPin = Node->FindPin(TEXT("ActorClass"));
        FString CurrentClassName = ClassPin && ClassPin->DefaultObject
            ? ClassPin->DefaultObject->GetPathName() : FString();

        // Если что-то изменилось - обновляем список
        if (CurrentFloorPath != CachedFloorAssetPath || CurrentClassName != CachedActorClassName)
        {
            CachedFloorAssetPath = CurrentFloorPath;
            CachedActorClassName = CurrentClassName;
            RefreshNameOptions();
        }

        return true;
    }

    void RefreshNameOptions()
    {
        NameOptions.Empty();
        CurrentSelection.Reset();

        if (!CachedNode.IsValid())
            return;

        UEdGraphNode* Node = CachedNode.Get();

        UEdGraphPin* FloorPin = Node->FindPin(TEXT("FloorAsset"));
        UFloorAsset* FloorAsset = nullptr;
        if (FloorPin && FloorPin->DefaultObject)
        {
            FloorAsset = Cast<UFloorAsset>(FloorPin->DefaultObject);
        }

        UEdGraphPin* ClassPin = Node->FindPin(TEXT("ActorClass"));
        TSubclassOf<AActor> ActorClass = nullptr;
        if (ClassPin && ClassPin->DefaultObject)
        {
            UClass* RawClass = Cast<UClass>(ClassPin->DefaultObject);
            if (RawClass && RawClass->IsChildOf(AActor::StaticClass()))
            {
                ActorClass = RawClass;
            }
        }

        if (FloorAsset)
        {
            UProxyManager* Manager = GetMutableDefault<UProxyManager>();
            TArray<UActorProxy*> Proxies = Manager->GetActorsFromFloorAsset(FloorAsset, ActorClass);

            for (UActorProxy* Proxy : Proxies)
            {
                if (Proxy)
                {
                    NameOptions.Add(MakeShared<FName>(Proxy->GetActorName()));
                }
            }
        }

        // Определяем текущее значение
        FString CurrentValue = GraphPinObj ? GraphPinObj->DefaultValue : FString();
        if (!CurrentValue.IsEmpty())
        {
            FName CurrentName(*CurrentValue);
            for (auto& Option : NameOptions)
            {
                if (Option.IsValid() && *Option == CurrentName)
                {
                    CurrentSelection = Option;
                    break;
                }
            }
        }
    }

protected:
    virtual TSharedRef<SWidget> GetDefaultValueWidget() override
    {
        // Первоначальное заполнение
        RefreshNameOptions();

        if (!CurrentSelection.IsValid() && NameOptions.Num() > 0)
        {
            CurrentSelection = NameOptions[0];
        }

        return SNew(SNameComboBox)
            .OptionsSource(&NameOptions)
            .InitiallySelectedItem(CurrentSelection)
            .OnSelectionChanged_Lambda([this](TSharedPtr<FName> NewSelection, ESelectInfo::Type SelectInfo)
                {
                    if (NewSelection.IsValid() && GraphPinObj)
                    {
                        CurrentSelection = NewSelection;
                        const FScopedTransaction Transaction(NSLOCTEXT("GraphEditor", "ChangeProxyActorName", "Change Proxy Actor Name"));
                        GraphPinObj->Modify();
                        GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, NewSelection->ToString());
                    }
                });
    }
};

TSharedPtr<SGraphPin> FProxyActorNamePinFactory::CreatePin(UEdGraphPin* Pin) const
{
    if (Pin && Pin->GetOwningNode() && Pin->GetOwningNode()->IsA<UK2Node_GetActorByClass>())
    {
        if (Pin->PinName == TEXT("ActorName"))
        {
            return SNew(SGraphPinProxyActorName, Pin);
        }
    }
    return nullptr;
}