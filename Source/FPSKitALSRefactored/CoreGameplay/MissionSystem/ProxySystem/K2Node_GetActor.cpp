#include "K2Node_GetActor.h"
#include "KismetCompiler.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "ProxyManager.h"
#include "ActorProxy.h"
#include "FloorAsset.h"

void UK2Node_GetActor::AllocateDefaultPins()
{
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Object, UFloorAsset::StaticClass(), TEXT("FloorAsset"));
    CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, TEXT("ActorName")); // здесь будет дроп‑даун
    CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, UActorProxy::StaticClass(), TEXT("Result"));
}

FText UK2Node_GetActor::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
    return FText::FromString("Get Proxy Actor");
}

FText UK2Node_GetActor::GetTooltipText() const
{
    return FText::FromString("Возвращает прокси‑актор из сцены по имени.");
}

void UK2Node_GetActor::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
    Super::ExpandNode(CompilerContext, SourceGraph);

    UFunction* TargetFunc = UProxyManager::StaticClass()->FindFunctionByName(TEXT("GetActorByName"));
    if (!TargetFunc)
    {
        CompilerContext.MessageLog.Error(TEXT("Function GetActorByName not found in UProxyManager"), this);
        BreakAllNodeLinks();
        return;
    }

    UK2Node_CallFunction* CallFuncNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
    CallFuncNode->SetFromFunction(TargetFunc);
    CallFuncNode->AllocateDefaultPins();

    CompilerContext.MovePinLinksToIntermediate(*FindPin(TEXT("FloorAsset")), *CallFuncNode->FindPin(TEXT("FloorAsset")));
    CompilerContext.MovePinLinksToIntermediate(*FindPin(TEXT("ActorName")), *CallFuncNode->FindPin(TEXT("ActorName")));
    CompilerContext.MovePinLinksToIntermediate(*FindPin(TEXT("Result")), *CallFuncNode->GetReturnValuePin());

    BreakAllNodeLinks();
}
