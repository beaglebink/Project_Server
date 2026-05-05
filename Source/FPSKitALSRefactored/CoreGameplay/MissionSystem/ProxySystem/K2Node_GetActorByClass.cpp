#include "K2Node_GetActorByClass.h"
#include "KismetCompiler.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "ProxyManager.h"
#include "ActorProxy.h"
#include "FloorAsset.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionFilter.h"

void UK2Node_GetActorByClass::AllocateDefaultPins()
{
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UFloorAsset::StaticClass(), TEXT("FloorAsset"));
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Class, AActor::StaticClass(), TEXT("ActorClass"));
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, TEXT("ActorName"));
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, UActorProxy::StaticClass(), TEXT("Result"));
}

void UK2Node_GetActorByClass::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
    Super::ReallocatePinsDuringReconstruction(OldPins);
    UpdateOutputPinType();
}

void UK2Node_GetActorByClass::PinDefaultValueChanged(UEdGraphPin* Pin)
{
    Super::PinDefaultValueChanged(Pin);

    if (Pin && (Pin->PinName == TEXT("ActorClass") || Pin->PinName == TEXT("FloorAsset")))
    {
        UpdateOutputPinType();
    }
}

TSubclassOf<AActor> UK2Node_GetActorByClass::GetActorClassFromPin() const
{
    UEdGraphPin* ClassPin = FindPin(TEXT("ActorClass"));
    if (ClassPin && ClassPin->DefaultObject)
    {
        UClass* RawClass = Cast<UClass>(ClassPin->DefaultObject);
        if (RawClass && RawClass->IsChildOf(AActor::StaticClass()))
        {
            return RawClass;
        }
    }
    return nullptr;
}

void UK2Node_GetActorByClass::UpdateOutputPinType()
{
    UEdGraphPin* ResultPin = FindPin(TEXT("Result"));
    if (!ResultPin) return;

    TSubclassOf<AActor> ActorClass = GetActorClassFromPin();

    if (ActorClass)
    {
        ResultPin->PinType.PinSubCategoryObject = ActorClass;
        // Используем FText::FromString для чистого имени
        ResultPin->PinFriendlyName = FText::FromString(ActorClass->GetName());
    }
    else
    {
        ResultPin->PinType.PinSubCategoryObject = UActorProxy::StaticClass();
        ResultPin->PinFriendlyName = FText::FromString(TEXT("Result"));
    }

    if (GetGraph())
    {
        GetGraph()->NotifyGraphChanged();
    }
}

FText UK2Node_GetActorByClass::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    TSubclassOf<AActor> ActorClass = GetActorClassFromPin();
    if (ActorClass)
    {
        // Чистый тайтл без printf-форматирования
        FString ClassName = ActorClass->GetName();
        return FText::FromString(FString(TEXT("Get ")) + ClassName + FString(TEXT(" from Floor")));
    }
    return NSLOCTEXT("K2Node", "GetProxyActorByClass", "Get Proxy Actor by Class");
}

FText UK2Node_GetActorByClass::GetTooltipText() const
{
    return NSLOCTEXT("K2Node", "GetProxyActorByClassTooltip", "Return a proxy actor of the specified class from the selected floor.");
}

void UK2Node_GetActorByClass::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);

    UFunction* TargetFunc = UProxyManager::StaticClass()->FindFunctionByName(TEXT("GetActorByName"));
    if (!TargetFunc)
    {
        CompilerContext.MessageLog.Error(TEXT("Function GetActorByName not found"), this);
        BreakAllNodeLinks();
        return;
    }

    UK2Node_CallFunction* CallFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    CallFuncNode->SetFromFunction(TargetFunc);
    CallFuncNode->AllocateDefaultPins();

    UEdGraphPin* FloorAssetPin = FindPin(TEXT("FloorAsset"));
    UEdGraphPin* ActorNamePin = FindPin(TEXT("ActorName"));

    if (FloorAssetPin)
    {
        CompilerContext.MovePinLinksToIntermediate(*FloorAssetPin, *CallFuncNode->FindPin(TEXT("FloorAsset")));
    }
    if (ActorNamePin)
    {
        CompilerContext.MovePinLinksToIntermediate(*ActorNamePin, *CallFuncNode->FindPin(TEXT("ActorName")));
    }

    UEdGraphPin* ResultPin = FindPin(TEXT("Result"));
    UEdGraphPin* FuncResultPin = CallFuncNode->GetReturnValuePin();

    if (ResultPin && FuncResultPin)
    {
        TSubclassOf<AActor> ActorClass = GetActorClassFromPin();
        if (ActorClass && ActorClass != AActor::StaticClass())
        {
            UK2Node_DynamicCast* CastNode = CompilerContext.SpawnIntermediateNode<UK2Node_DynamicCast>(this, SourceGraph);
            CastNode->TargetType = ActorClass;
            CastNode->AllocateDefaultPins();

            FuncResultPin->MakeLinkTo(CastNode->GetCastSourcePin());

            UEdGraphPin* CastResultPin = CastNode->GetCastResultPin();
            if (CastResultPin)
            {
                CompilerContext.MovePinLinksToIntermediate(*ResultPin, *CastResultPin);
            }
        }
        else
        {
            CompilerContext.MovePinLinksToIntermediate(*ResultPin, *FuncResultPin);
        }
    }

    BreakAllNodeLinks();
}

void UK2Node_GetActorByClass::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UClass* ActionKey = GetClass();

    if (ActionRegistrar.IsOpenForRegistration(ActionKey))
    {
        UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
        check(NodeSpawner != nullptr);
        ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
    }
}

FText UK2Node_GetActorByClass::GetMenuCategory() const
{
    return NSLOCTEXT("K2Node", "GetProxyActorByClassCategory", "Proxy System");
}