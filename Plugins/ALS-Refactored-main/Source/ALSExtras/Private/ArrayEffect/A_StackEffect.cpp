#include "ArrayEffect/A_StackEffect.h"
#include "ArrayEffect/A_ArrayEffect.h"
#include "ArrayEffect/A_ArrayNode.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AlsCharacterExample.h"
#include "AlsCameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"

AA_StackEffect::AA_StackEffect()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AA_StackEffect::BeginPlay()
{
	Super::BeginPlay();
}

void AA_StackEffect::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_StackEffect::GetTextCommand(FText Command)
{
	Super::GetTextCommand(Command);
}

void AA_StackEffect::AppendNode(FName VariableName)
{
	if (NodeArray.Num() == 10)
	{
		return;
	}

	EndNode->SetIndex(NodeArray.Num());

	if (!VariableName.IsNone())
	{
		if (AActor* GrabbedActor = GetActorWithTag(VariableName))
		{
			GrabbedActor->Tags.Remove(VariableName);
			EndNode->TryGrabActor(GrabbedActor);
		}
	}

	FVector SpawnLocation = EndNode->GetActorLocation();
	AA_ArrayNode* NewNode = EndNode;
	NodeArray.Add(NewNode);
	if (NodeClass)
	{
		EndNode = GetWorld()->SpawnActor<AA_ArrayNode>(NodeClass, SpawnLocation, GetActorRotation());
		if (EndNode)
		{
			//EndNode->OwnerActor = this;
			EndNode->MoveNode(EndNode->GetActorLocation() - GetActorRightVector() * NodeWidth);
		}
	}
}

bool AA_StackEffect::ParseCommandToAppend(FText Command, FText& PrevName, FName& VariableName)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	FString ParsedArrayName, MethodPart;
	if (!Input.Split(TEXT("."), &ParsedArrayName, &MethodPart))
	{
		return false;

	}
	if (!AA_PythonContainer::IsValidPythonIdentifier(ParsedArrayName))
	{
		return false;
	}
	PrevName = FText::FromString(ParsedArrayName);

	const FString Prefix = TEXT("append(");
	const FString Suffix = TEXT(")");

	if (!MethodPart.StartsWith(Prefix) || !MethodPart.EndsWith(Suffix))
	{
		return false;
	}

	FString Inside = MethodPart.Mid(Prefix.Len(), MethodPart.Len() - Prefix.Len() - 1);
	if (!Inside.IsEmpty())
	{
		if (!AA_PythonContainer::IsValidPythonIdentifier(Inside))
		{
			return false;
		}
		VariableName = FName(*Inside);
	}

	return true;
}

bool AA_StackEffect::ParseCommandToPop(FText Command, FText& PrevName, FName& VariableName, int32& Index)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	FString Left, Right;
	if (!Input.Split(TEXT("="), &Left, &Right))
	{
		Left.Empty();
		Right = Input;
	}

	if (!Left.IsEmpty())
	{
		if (!AA_PythonContainer::IsValidPythonIdentifier(Left))
		{
			return false;
		}
		VariableName = FName(Left);
	}

	const FString PopPrefix = TEXT(".pop(");
	const FString PopSuffix = TEXT(")");

	int32 PrefixPos = Right.Find(PopPrefix);
	if (PrefixPos == INDEX_NONE || !Right.EndsWith(PopSuffix))
	{
		return false;
	}

	FString ParsedArrayName = Right.Left(PrefixPos);
	if (!AA_PythonContainer::IsValidPythonIdentifier(ParsedArrayName))
	{
		return false;
	}
	PrevName = FText::FromString(ParsedArrayName);

	FString Inside = Right.Mid(PrefixPos + PopPrefix.Len(), Right.Len() - (PrefixPos + PopPrefix.Len() + 1));

	if (Inside.IsEmpty())
	{
		Index = NodeArray.Num() - 1;
	}
	else
	{
		if (!Inside.IsNumeric())
		{
			return false;
		}

		Index = FCString::Atoi(*Inside);
		if (Index < 0 || Index >= NodeArray.Num())
		{
			return false;
		}
	}

	return true;
}