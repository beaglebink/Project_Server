#include "ArrayEffect/A_QueueEffect.h"
#include "ArrayEffect/A_ArrayEffect.h"
#include "ArrayEffect/A_ArrayNode.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AlsCharacterExample.h"
#include "AlsCameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"

AA_QueueEffect::AA_QueueEffect()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AA_QueueEffect::BeginPlay()
{
	Super::BeginPlay();
}

void AA_QueueEffect::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_QueueEffect::GetTextCommand(FText Command)
{
	Super::GetTextCommand(Command);

}

void AA_QueueEffect::AppendNode(FName VariableName)
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

void AA_QueueEffect::ContainerPop(int32 Index)
{
	DeleteNode(Index);
}

void AA_QueueEffect::ContainerClear()
{
	for (int32 i = NodeArray.Num() - 1; i >= 0; --i)
	{
		DeleteNode(i);
	}
}

void AA_QueueEffect::ContainerExtend()
{
	if (EndNode->bIsMoving)
	{
		return;
	}

	if (ExtendContainerIndex < ExtendContainer.Num())
	{
		++ExtendContainerIndex;
		AppendNode();
	}
	else
	{
		ExtendContainer.Empty();
		ExtendContainerIndex = 0;
	}
}

bool AA_QueueEffect::ParseCommandToAppend(FText Command, FText& PrevName, FName& VariableName)
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

bool AA_QueueEffect::ParseCommandToDelete(FText Command, FText& PrevName, int32& OutIndex, int32& OutLeftIndex, int32& OutRightIndex)
{
	OutIndex = INDEX_NONE;
	OutLeftIndex = INDEX_NONE;
	OutRightIndex = INDEX_NONE;

	FString Input = Command.ToString();

	const FString DelPrefix = TEXT("del ");
	if (!Input.StartsWith(DelPrefix))
	{
		return false;
	}
	Input.RemoveSpacesInline();

	FString Remainder = Input.RightChop(DelPrefix.Len() - 1);
	if (Remainder.IsEmpty())
	{
		return false;
	}

	//del arrname
	int32 FirstBracketPos = INDEX_NONE;
	if (!Remainder.FindChar(TEXT('['), FirstBracketPos))
	{
		if (!AA_PythonContainer::IsValidPythonIdentifier(Remainder))
		{
			return false;
		}
		PrevName = FText::FromString(Remainder);
		return true;
	}

	//del arrname[...]
	int32 CloseBracketPos = INDEX_NONE;
	if (!Remainder.FindLastChar(TEXT(']'), CloseBracketPos) || CloseBracketPos <= FirstBracketPos || CloseBracketPos != Remainder.Len() - 1)
	{
		return false;
	}

	FString ParsedArrayName = Remainder.Left(FirstBracketPos);
	if (!AA_PythonContainer::IsValidPythonIdentifier(ParsedArrayName))
	{
		return false;
	}
	PrevName = FText::FromString(ParsedArrayName);

	FString Inside = Remainder.Mid(FirstBracketPos + 1, CloseBracketPos - FirstBracketPos - 1);

	//del arrname[index]
	if (Inside.IsNumeric())
	{
		OutIndex = FCString::Atoi(*Inside);
		return OutIndex < NodeArray.Num();
	}

	//slices ':'
	int32 ColonPos = INDEX_NONE;
	if (!Inside.FindChar(TEXT(':'), ColonPos))
	{
		return false;
	}

	FString Left = Inside.Left(ColonPos);
	FString Right = Inside.Mid(ColonPos + 1);

	if (Left.IsEmpty() && Right.IsEmpty()) // [:]
	{
		OutLeftIndex = 0;
		OutRightIndex = NodeArray.Num();
		return true;
	}
	else if (Left.IsEmpty() && Right.IsNumeric()) // [:index]
	{
		OutLeftIndex = 0;
		OutRightIndex = FCString::Atoi(*Right);
		return OutRightIndex < NodeArray.Num();
	}
	else if (Right.IsEmpty() && Left.IsNumeric()) // [index:]
	{
		OutLeftIndex = FCString::Atoi(*Left);
		OutRightIndex = NodeArray.Num();
		return OutLeftIndex < NodeArray.Num();
	}
	else if (Left.IsNumeric() && Right.IsNumeric()) // [index:index]
	{
		OutLeftIndex = FCString::Atoi(*Left);
		OutRightIndex = FCString::Atoi(*Right);
		return OutLeftIndex < OutRightIndex && OutRightIndex < NodeArray.Num();
	}

	return false;
}

bool AA_QueueEffect::ParseCommandToPop(FText Command, FText& PrevName, FName& VariableName, int32& Index)
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