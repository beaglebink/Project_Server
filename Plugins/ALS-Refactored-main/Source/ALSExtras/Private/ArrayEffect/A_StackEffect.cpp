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

	ContainerExtend();

	AttachToCharacterCamera();

	DetachFromCharacterCamera();

	AttachToContainer();

	RefreshNameLocationAndRotation();
}

void AA_StackEffect::GetTextCommand(FText Command)
{
	if (bIsSwapping || EndNode->bIsMoving || bIsDetaching)
	{
		return;
	}

	int32 OutIndex = -1;

	int32 OutLeftIndex = -1;

	int32 OutRightIndex = -1;

	int32 SizeOfConcatenatedArray = -1;

	FText PrevName;

	FText NewName;

	FName VariableName = NAME_None;

	int32 ArrayNum;

	//append
	if (ParseCommandToAppend(Command, PrevName, VariableName) && PrevName.ToString() == ContainerName.ToString())
	{
		AppendNode(VariableName);
	}
	//swap
	else if (ParseCommandToSwap(Command, PrevName, OutLeftIndex, OutRightIndex) && PrevName.ToString() == ContainerName.ToString())
	{
		SwapNodes(OutLeftIndex, OutRightIndex);
	}
	//delete
	else if (ParseCommandToDelete(Command, PrevName, OutIndex, OutLeftIndex, OutRightIndex) && PrevName.ToString() == ContainerName.ToString())
	{
		if (OutIndex != -1)
		{
			DeleteNode(OutIndex);
		}
		else if (OutLeftIndex != -1 && OutRightIndex != -1)
		{
			for (size_t i = OutLeftIndex; i < OutRightIndex; ++i)
			{
				DeleteNode(OutLeftIndex);
			}
		}
		else
		{
			ContainerClear();
			EndNode->Destroy();
			Destroy();
		}
		MoveNodesConsideringOrder();
	}
	//insert
	else if (ParseCommandToInsert(Command, PrevName, VariableName, OutIndex) && PrevName.ToString() == ContainerName.ToString())
	{
		InsertNode(VariableName, OutIndex);
	}
	//pop
	else if (ParseCommandToPop(Command, PrevName, VariableName, OutIndex) && PrevName.ToString() == ContainerName.ToString())
	{
		if (VariableName.ToString() != "")
		{
			NodeArray[OutIndex]->GrabbedActor->Tags.AddUnique(VariableName);
		}
		ContainerPop(OutIndex);
		MoveNodesConsideringOrder();
	}
	//clear
	else if (ParseCommandToClear(Command, PrevName) && PrevName.ToString() == ContainerName.ToString())
	{
		ContainerClear();
		MoveNodesConsideringOrder();
	}
	//extend
	else if (ParseCommandToExtend(Command, PrevName, ExtendContainer) && PrevName.ToString() == ContainerName.ToString())
	{
	}
	//concatenate
	else if (ParseCommandToConcatenate(Command, SizeOfConcatenatingContainer, SizeOfConcatenatedArray))
	{
		if (NodeArray.Num() != SizeOfConcatenatedArray)
		{
			return;
		}

		bIsOnConcatenation = true;
	}
	//reset concatenation
	else if (bIsOnConcatenation && ParseCommandToReset(Command, PrevName) && PrevName.ToString() == ContainerName.ToString())
	{
		bIsOnConcatenation = false;
		bIsDetaching = true;;
	}
	//rename
	else if (ParseCommandToRename(Command, PrevName, NewName, ArrayNum) && PrevName.ToString() == ContainerName.ToString() && ArrayNum == NodeArray.Num())
	{
		ContainerRename(NewName);
	}
	//copy
	else if (ParseCommandToCopy(Command, PrevName, NewName, OutLeftIndex, OutRightIndex) && PrevName.ToString() == ContainerName.ToString())
	{
		ContainerCopy(NewName, OutLeftIndex, OutRightIndex);
	}
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

void AA_StackEffect::DeleteNode(int32 Index)
{
	if (NodeArray.IsEmpty())
	{
		return;
	}

	NodeArray[Index]->DeleteNode();

	for (size_t i = Index + 1; i < NodeArray.Num(); ++i)
	{
		NodeArray[i]->SetIndex(i - 1);
	}

	NodeArray.RemoveAt(Index);
}

void AA_StackEffect::InsertNode(FName VariableName, int32 Index)
{
	if (NodeArray.Num() == 10)
	{
		return;
	}

	EndNode->MoveNode(EndNode->GetActorLocation() - GetActorRightVector() * NodeWidth);

	FVector SpawnLocation = GetActorLocation();
	if (NodeArray.Num() > 0)
	{
		SpawnLocation = NodeArray[Index]->GetActorLocation();
	}

	for (size_t i = Index; i < NodeArray.Num(); ++i)
	{
		NodeArray[i]->MoveNode(NodeArray[i]->GetActorLocation() - GetActorRightVector() * NodeWidth);
		NodeArray[i]->SetIndex(i + 1);
	}

	AA_ArrayNode* NewNode = GetWorld()->SpawnActor<AA_ArrayNode>(NodeClass, SpawnLocation, GetActorRotation());
	if (NewNode)
	{
		//NewNode->OwnerActor = this;
		NewNode->SetIndex(Index);
		NodeArray.Insert(NewNode, Index);

		if (!VariableName.IsNone())
		{
			if (AActor* GrabbedActor = GetActorWithTag(VariableName))
			{
				GrabbedActor->Tags.Remove(VariableName);
				NewNode->TryGrabActor(GrabbedActor);
			}
		}
	}
}

void AA_StackEffect::ContainerPop(int32 Index)
{
	DeleteNode(Index);
}

void AA_StackEffect::ContainerClear()
{
	for (int32 i = NodeArray.Num() - 1; i >= 0; --i)
	{
		DeleteNode(i);
	}
}

void AA_StackEffect::ContainerExtend()
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

void AA_StackEffect::ContainerConcatenate(AA_StackEffect* ArrayToConcatenate)
{
	if (NodeArray.Num() != ArrayToConcatenate->SizeOfConcatenatingContainer)
	{
		ArrayToConcatenate->bIsOnConcatenation = false;
		ArrayToConcatenate->bIsDetaching = true;
		return;
	}

	ArrayToConcatenate->bIsOnConcatenation = false;

	int32 Index = NodeArray.Num();
	for (AA_ArrayNode* Node : ArrayToConcatenate->NodeArray)
	{
		//Node->OwnerActor = this;
		Node->DefaultLocation = GetActorLocation() - GetActorRightVector() * NodeWidth * Index;
		Node->SetIndex(Index++);
	}

	ArrayToConcatenate->EndNode->Destroy();
	NodeArray.Append(ArrayToConcatenate->NodeArray);
	EndNode->DefaultLocation = NodeArray.Last()->DefaultLocation - GetActorRightVector() * NodeWidth;
	ArrayToConcatenate->Destroy();

	bIsAttaching = true;
}

void AA_StackEffect::ContainerRename(FText NewName)
{
	SetContainerName(NewName);
}

void AA_StackEffect::ContainerCopy(FText Name, int32 OutLeftIndex, int32 OutRightIndex)
{
	if (!ContainerClass || OutLeftIndex == -1 || OutRightIndex == -1)
	{
		return;
	}

	if (AA_StackEffect* NewArray = GetWorld()->SpawnActor<AA_StackEffect>(ContainerClass, EndNode->DefaultLocation - GetActorRightVector() * NodeWidth * 2, GetActorRotation()))
	{
		NewArray->DefaultLocation = NewArray->GetActorLocation();
		NewArray->SetContainerName(Name);

		int32 Index = 0;
		for (size_t i = OutLeftIndex; i < OutRightIndex; ++i)
		{
			if (AA_ArrayNode* NewNode = GetWorld()->SpawnActor<AA_ArrayNode>(NodeClass, NodeArray[i]->GetActorLocation(), GetActorRotation()))
			{
				//NewNode->OwnerActor = NewArray;
				NewArray->NodeArray.Add(NewNode);
				NewNode->SetIndex(Index++);
				if (NodeArray[i]->GrabbedActor)
				{
					if (AActor* CopyGrabbedActor = GetWorld()->SpawnActor<AActor>(NodeArray[i]->GrabbedActor->GetClass(), NewNode->GetActorLocation(), NewNode->GetActorRotation()))
					{
						NewNode->AttachToNode(CopyGrabbedActor);
						NewNode->GrabbedActor = CopyGrabbedActor;
						if (UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(CopyGrabbedActor->GetComponentByClass(UStaticMeshComponent::StaticClass())))
						{
							NewNode->GrabbedComponent = MeshComp;
						}
					}
				}
			}
		}
		NewArray->MoveNodesConsideringOrder();
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

bool AA_StackEffect::ParseCommandToSwap(FText Command, FText& PrevName, int32& OutIndex1, int32& OutIndex2)
{
	FString Input = Command.ToString();


	FString Left, Right;
	if (!Input.Split(TEXT("="), &Left, &Right))
	{
		return false;
	}

	TArray<int32> LeftIndices;
	TArray<int32> RightIndices;
	FString ParsedArrayName;

	auto ParseIndices = [this, &ParsedArrayName, &PrevName](const FString& Part, TArray<int32>& OutIndices) -> bool
		{
			TArray<FString> Elements;
			Part.ParseIntoArray(Elements, TEXT(","), true);

			if (OutIndices.Num() != 2)
			{
				return false;
			}

			for (const FString& Elem : Elements)
			{
				FString Name, IndexStr;

				if (!Elem.Split(TEXT("["), &Name, &IndexStr))
					return false;

				if (!IndexStr.EndsWith(TEXT("]")))
					return false;

				IndexStr.RemoveAt(IndexStr.Len() - 1);

				if (!IndexStr.IsNumeric())
					return false;

				if (ParsedArrayName.IsEmpty())
				{
					if (AA_PythonContainer::IsValidPythonIdentifier(Name))
					{
						ParsedArrayName = Name;
						PrevName = FText::FromString(ParsedArrayName);
					}
					else
					{
						return false;
					}
				}
				else
				{
					if (Name != ParsedArrayName)
						return false;
				}

				int32 Index = FCString::Atoi(*IndexStr);
				OutIndices.Add(Index);
			}
			return true;
		};

	if (ParseIndices(Left, LeftIndices) && ParseIndices(Right, RightIndices) && LeftIndices[0] == RightIndices[1] && LeftIndices[1] == RightIndices[0])
	{
		OutIndex1 = LeftIndices[0];
		OutIndex2 = LeftIndices[1];

		return true;
	}

	return false;
}

bool AA_StackEffect::ParseCommandToDelete(FText Command, FText& PrevName, int32& OutIndex, int32& OutLeftIndex, int32& OutRightIndex)
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

bool AA_StackEffect::ParseCommandToInsert(FText Command, FText& PrevName, FName& VariableName, int32& OutIndex)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	const FString InsertPrefix = TEXT(".insert(");
	if (!Input.Contains(InsertPrefix) || !Input.EndsWith(TEXT(")")))
	{
		return false;
	}

	int32 PrefixPos = Input.Find(InsertPrefix);
	FString ParsedArrayName = Input.Left(PrefixPos);

	if (!AA_PythonContainer::IsValidPythonIdentifier(ParsedArrayName))
	{
		return false;
	}
	PrevName = FText::FromString(ParsedArrayName);

	FString Inside = Input.Mid(PrefixPos + InsertPrefix.Len(), Input.Len() - (PrefixPos + InsertPrefix.Len() + 1));

	TArray<FString> Parts;
	Inside.ParseIntoArray(Parts, TEXT(","), true);
	if (Parts.Num() != 2)
	{
		return false;
	}

	if (!Parts[0].IsNumeric())
	{
		return false;
	}
	OutIndex = FCString::Atoi(*Parts[0]);
	if (OutIndex >= NodeArray.Num())
	{
		OutIndex = FMath::Max(0, NodeArray.Num() - 1);
	}

	FString VarStr = Parts[1];
	if (VarStr == TEXT("None") || VarStr == TEXT("''") || VarStr == TEXT("[]"))
	{
		VariableName = FName();
	}
	else
	{
		if (!AA_PythonContainer::IsValidPythonIdentifier(VarStr))
		{
			return false;
		}
		VariableName = FName(*VarStr);
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

bool AA_StackEffect::ParseCommandToClear(FText Command, FText& PrevName)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	const FString Suffix = TEXT(".clear()");
	if (!Input.EndsWith(Suffix))
	{
		return false;
	}

	FString ParsedArrayName = Input.LeftChop(Suffix.Len());

	if (!AA_PythonContainer::IsValidPythonIdentifier(ParsedArrayName))
	{
		return false;
	}

	PrevName = FText::FromString(ParsedArrayName);
	return true;
}

bool AA_StackEffect::ParseCommandToExtend(FText Command, FText& PrevName, TArray<int32>& OutArray)
{
	OutArray.Empty();
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	const FString Prefix = TEXT(".extend(");
	const FString Suffix = TEXT(")");

	int32 ExtendPos = Input.Find(Prefix);
	if (ExtendPos == INDEX_NONE || !Input.EndsWith(Suffix))
	{
		return false;
	}

	FString ParsedArrayName = Input.Left(ExtendPos);
	if (!AA_PythonContainer::IsValidPythonIdentifier(ParsedArrayName))
	{
		return false;
	}
	PrevName = FText::FromString(ParsedArrayName);

	FString Inner = Input.Mid(ExtendPos + Prefix.Len(), Input.Len() - (ExtendPos + Prefix.Len()) - Suffix.Len());

	TArray<FString> Parts;
	Inner.ParseIntoArray(Parts, TEXT(","), true);

	if (Parts.Num() == 0 && Inner.Len() > 0)
	{
		Parts.Add(Inner);
	}

	for (FString& Part : Parts)
	{
		if (!Part.StartsWith(TEXT("[")) || !Part.EndsWith(TEXT("]")))
		{
			OutArray.Empty();
			return false;
		}

		FString NumberStr = Part.Mid(1, Part.Len() - 2);

		if (!NumberStr.IsNumeric())
		{
			OutArray.Empty();
			return false;
		}

		OutArray.Add(FCString::Atoi(*NumberStr));
	}

	return true;
}

bool AA_StackEffect::ParseCommandToConcatenate(FText Command, int32& OutSize1, int32& OutSize2)
{
	FString Input = Command.ToString();

	OutSize1 = 0;
	OutSize2 = 0;

	Input.RemoveSpacesInline();

	if (!Input.StartsWith(TEXT("arr=")))
		return false;

	int32 PlusIndex;
	if (!Input.FindChar('+', PlusIndex))
		return false;

	FString Left = Input.Mid(4, PlusIndex - 4);
	FString Right = Input.Mid(PlusIndex + 1);

	auto CountArrayElements = [](const FString& Part) -> int32
		{
			if (!Part.StartsWith(TEXT("[")) || !Part.EndsWith(TEXT("]")))
				return -1;

			FString Inner = Part.Mid(1, Part.Len() - 2);
			if (Inner.IsEmpty())
				return 0;

			TArray<FString> Elements;
			Inner.ParseIntoArray(Elements, TEXT(","), true);
			return Elements.Num();
		};

	OutSize1 = CountArrayElements(Left);
	OutSize2 = CountArrayElements(Right);

	return (OutSize1 > 0 && OutSize2 > 0 && (OutSize1 + OutSize2 <= 10));
}

bool AA_StackEffect::ParseCommandToReset(FText Command, FText& PrevName)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	const FString Suffix = TEXT(".reset()");
	if (!Input.EndsWith(Suffix))
	{
		return false;
	}

	FString ParsedArrayName = Input.LeftChop(Suffix.Len());

	if (!AA_PythonContainer::IsValidPythonIdentifier(ParsedArrayName))
	{
		return false;
	}

	PrevName = FText::FromString(ParsedArrayName);
	return true;
}

bool AA_StackEffect::ParseCommandToRename(FText Command, FText& PrevName, FText& NewName, int32& ArrayNum)
{
	const FString Input = Command.ToString();

	TArray<FString> Lines;
	Input.ParseIntoArrayLines(Lines);
	if (Lines.Num() != 3)
	{
		return false;
	}

	FString Left, Right;

	Lines[0].RemoveSpacesInline();
	if (!Lines[0].Split(TEXT("="), &Left, &Right) || Left.IsEmpty() || !Right.StartsWith(TEXT("[")) || !Right.EndsWith(TEXT("]")))
	{
		return false;
	}
	const FString ParsedArrayName = Left;
	FString Inner = Right.Mid(1, Right.Len() - 2);
	TArray<FString> Elements;
	Inner.ParseIntoArray(Elements, TEXT(","), true);
	ArrayNum = 0;
	for (const FString& Elem : Elements)
	{
		if (Elem.IsEmpty() || !Elem.IsNumeric())
		{
			return false;
		}
		++ArrayNum;
	}

	Lines[1].RemoveSpacesInline();
	if (!Lines[1].Split(TEXT("="), &Left, &Right) || Left.IsEmpty() || Right.IsEmpty() || Right != ParsedArrayName)
	{
		return false;
	}
	if (!AA_PythonContainer::IsValidPythonIdentifier(Left))
	{
		return false;
	}
	NewName = FText::FromString(Left);

	if (!Lines[2].StartsWith(TEXT("del ")))
	{
		return false;
	}
	FString DelName = Lines[2].Mid(4).TrimStartAndEnd();
	if (DelName.IsEmpty() || DelName != ParsedArrayName)
	{
		return false;
	}
	PrevName = FText::FromString(DelName);

	return true;
}

bool AA_StackEffect::ParseCommandToCopy(FText Command, FText& PrevName, FText& CopyName, int32& OutLeftIndex, int32& OutRightIndex)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();
	FString Left, Right;

	if (!Input.Split(TEXT("="), &Left, &Right))
	{
		return false;
	}

	if (Left.IsEmpty() || Right.IsEmpty())
	{
		return false;
	}

	FString BaseName;
	OutLeftIndex = -1;
	OutRightIndex = -1;
	int32 OpenBracket, CloseBracket;

	if (Right.EndsWith(TEXT(".copy()")))
	{
		BaseName = Right.LeftChop(7);
		OutLeftIndex = 0;
		OutRightIndex = NodeArray.Num();
	}
	else if (Right.StartsWith(TEXT("list(")) && Right.EndsWith(TEXT(")")))
	{
		BaseName = Right.Mid(5, Right.Len() - 6);
		OutLeftIndex = 0;
		OutRightIndex = NodeArray.Num();
	}
	else if (Right.FindChar(TEXT('['), OpenBracket) && Right.FindLastChar(TEXT(']'), CloseBracket))
	{
		BaseName = Right.Left(OpenBracket);
		FString Inside = Right.Mid(OpenBracket + 1, CloseBracket - OpenBracket - 1);

		int32 ColonPos;
		if (!Inside.FindChar(TEXT(':'), ColonPos))
		{
			return false;
		}

		FString LeftSlice = Inside.Left(ColonPos);
		FString RightSlice = Inside.Mid(ColonPos + 1);

		if (LeftSlice.IsEmpty() && RightSlice.IsEmpty()) // [:]
		{
			OutLeftIndex = 0;
			OutRightIndex = NodeArray.Num();
		}
		else if (LeftSlice.IsEmpty() && RightSlice.IsNumeric()) // [:index]
		{
			OutLeftIndex = 0;
			OutRightIndex = FCString::Atoi(*RightSlice);
			if (OutRightIndex >= NodeArray.Num())
			{
				return false;
			}
		}
		else if (RightSlice.IsEmpty() && LeftSlice.IsNumeric()) // [index:]
		{
			OutLeftIndex = FCString::Atoi(*LeftSlice);
			OutRightIndex = NodeArray.Num();
			if (OutLeftIndex >= NodeArray.Num())
			{
				return false;
			}
		}
		else if (LeftSlice.IsNumeric() && RightSlice.IsNumeric()) // [index:index]
		{
			OutLeftIndex = FCString::Atoi(*LeftSlice);
			OutRightIndex = FCString::Atoi(*RightSlice);
			if (OutLeftIndex >= OutRightIndex || OutRightIndex >= NodeArray.Num())
			{
				return false;
			}
		}
	}
	else
	{
		return false;
	}

	if (!AA_PythonContainer::IsValidPythonIdentifier(Left) || !AA_PythonContainer::IsValidPythonIdentifier(BaseName))
	{
		return false;
	}

	if (Left == BaseName)
	{
		return false;
	}

	PrevName = FText::FromString(BaseName);
	CopyName = FText::FromString(Left);

	return true;
}

void AA_StackEffect::AttachToCharacterCamera()
{
	if (!bIsOnConcatenation)
	{
		return;
	}

	if (AAlsCharacterExample* PC = Cast< AAlsCharacterExample>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		FMinimalViewInfo MinimalViewInfo;
		PC->CalcCamera(0.f, MinimalViewInfo);
		const FVector Location = MinimalViewInfo.Location;
		const FRotator Rotation = MinimalViewInfo.Rotation;
		const FVector Direction = Rotation.Vector();

		SetActorLocation(FMath::VInterpTo(GetActorLocation(), Location + Direction * 800.0f, GetWorld()->GetDeltaSeconds(), 2.0f));
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), (-Direction).Rotation(), GetWorld()->GetDeltaSeconds(), 2.0f));

		NodeArray[0]->SetActorLocation(FMath::VInterpTo(NodeArray[0]->GetActorLocation(), GetActorLocation(), GetWorld()->GetDeltaSeconds(), 2.0f));
		NodeArray[0]->SetActorRotation(FMath::RInterpTo(NodeArray[0]->GetActorRotation(), (-Direction).Rotation(), GetWorld()->GetDeltaSeconds(), 2.0f));

		for (int32 Index = 1; Index < NodeArray.Num(); ++Index)
		{
			NodeArray[Index]->SetActorLocation(FMath::VInterpTo(NodeArray[Index]->GetActorLocation(), NodeArray[Index - 1]->GetActorLocation() - NodeArray[Index - 1]->GetActorRightVector() * NodeWidth, GetWorld()->GetDeltaSeconds(), 2.0f));
			NodeArray[Index]->SetActorRotation(FMath::RInterpTo(NodeArray[Index]->GetActorRotation(), (-Direction).Rotation(), GetWorld()->GetDeltaSeconds(), 2.0f));
		}
		EndNode->SetActorLocation(FMath::VInterpTo(EndNode->GetActorLocation(), NodeArray.Last()->GetActorLocation() - NodeArray.Last()->GetActorRightVector() * NodeWidth, GetWorld()->GetDeltaSeconds(), 2.0f));
		EndNode->SetActorRotation(FMath::RInterpTo(EndNode->GetActorRotation(), (-Direction).Rotation(), GetWorld()->GetDeltaSeconds(), 2.0f));
	}
}

void AA_StackEffect::DetachFromCharacterCamera()
{
	if (!bIsDetaching)
	{
		return;
	}

	if (GetActorLocation().Equals(DefaultLocation, 0.01f) && GetActorRotation().Equals(DefaultRotation, 0.01f))
	{
		bIsDetaching = false;
	}

	SetActorLocation(FMath::VInterpTo(GetActorLocation(), DefaultLocation, GetWorld()->GetDeltaSeconds(), 2.0f));
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), DefaultRotation, GetWorld()->GetDeltaSeconds(), 2.0f));
	for (AA_ArrayNode* Node : NodeArray)
	{
		Node->SetActorLocation(FMath::VInterpTo(Node->GetActorLocation(), Node->DefaultLocation, GetWorld()->GetDeltaSeconds(), 2.0f));
		Node->SetActorRotation(FMath::RInterpTo(Node->GetActorRotation(), DefaultRotation, GetWorld()->GetDeltaSeconds(), 2.0f));
	}
	EndNode->SetActorLocation(FMath::VInterpTo(EndNode->GetActorLocation(), EndNode->DefaultLocation, GetWorld()->GetDeltaSeconds(), 2.0f));
	EndNode->SetActorRotation(FMath::RInterpTo(EndNode->GetActorRotation(), DefaultRotation, GetWorld()->GetDeltaSeconds(), 2.0f));
}

void AA_StackEffect::AttachToContainer()
{
	if (!bIsAttaching)
	{
		return;
	}

	if (EndNode->GetActorLocation().Equals(EndNode->DefaultLocation, 0.01f) && EndNode->GetActorRotation().Equals(DefaultRotation, 0.01f))
	{
		bIsAttaching = false;
	}

	for (AA_ArrayNode* Node : NodeArray)
	{
		Node->SetActorLocation(FMath::VInterpTo(Node->GetActorLocation(), Node->DefaultLocation, GetWorld()->GetDeltaSeconds(), 2.0f));
		Node->SetActorRotation(FMath::RInterpTo(Node->GetActorRotation(), DefaultRotation, GetWorld()->GetDeltaSeconds(), 2.0f));
	}
	EndNode->SetActorLocation(FMath::VInterpTo(EndNode->GetActorLocation(), EndNode->DefaultLocation, GetWorld()->GetDeltaSeconds(), 2.0f));
	EndNode->SetActorRotation(FMath::RInterpTo(EndNode->GetActorRotation(), DefaultRotation, GetWorld()->GetDeltaSeconds(), 2.0f));
}

void AA_StackEffect::MoveNodesConsideringOrder()
{
	for (size_t i = 0; i < NodeArray.Num(); ++i)
	{
		NodeArray[i]->MoveNode(GetActorLocation() - GetActorRightVector() * NodeWidth * i);
	}
	if (IsValid(EndNode))
	{
		EndNode->MoveNode(GetActorLocation() - GetActorRightVector() * NodeWidth * NodeArray.Num());
	}
}

void AA_StackEffect::RefreshNameLocationAndRotation()
{
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);

	if (!EndNode || !Player)
	{
		return;
	}

	FVector Start = GetActorLocation();
	FVector End = EndNode->GetActorLocation();
	//FVector LineDir = (End - Start).GetSafeNormal();
	//float Dot = FVector::DotProduct(Player->GetActorLocation() - Start, LineDir);
	//float ClampedDist = FMath::Clamp(Dot, 0.f, FVector::Dist(Start, End));
	//FVector ClampedPos = Start + LineDir * ClampedDist;
	//FVector PosToPlayerDir = Player->GetActorLocation() - ClampedPos;
	//PosToPlayerDir.Normalize();
	//ClampedPos += PosToPlayerDir * 100.0f;
	//ClampedPos.Z = GetActorLocation().Z - 140.0f + FMath::Sin(GetWorld()->GetTimeSeconds()) * 10.0f;
	//FVector NameLocation = FMath::VInterpTo(TextComp->GetComponentLocation(), ClampedPos, GetWorld()->GetDeltaSeconds(), 2.0f);
	//TextComp->SetWorldLocation(NameLocation);
	FVector NameLocation = (Start + End) * 0.5f;
	NameLocation.Z -= 140;
	TextComp->SetWorldLocation(NameLocation);

	//FRotator NameRotation = (Player->GetActorLocation() - TextComp->GetComponentLocation()).Rotation();
	//TextComp->SetWorldRotation(NameRotation);
}

void AA_StackEffect::SetContainerName(FText Name)
{
	ContainerName = Name;
	TextComp->SetText(Name);
}

AActor* AA_StackEffect::GetActorWithTag(const FName& Tag)
{
	if (!GetWorld())
	{
		return nullptr;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), Tag, FoundActors);

	return FoundActors.Num() > 0 ? FoundActors[0] : nullptr;
}

void AA_StackEffect::TimelineProgress(float Value)
{
	FVector FirstBase = FMath::Lerp(NodeArray[SwapNode1]->GetActorLocation(), NodeArray[SwapNode2]->GetActorLocation(), Value);
	FVector SecondBase = FMath::Lerp(NodeArray[SwapNode2]->GetActorLocation(), NodeArray[SwapNode1]->GetActorLocation(), Value);

	float Height = HeightFloatCurve->GetFloatValue(Value) * 200.0f;

	FirstBase.Z += Height;
	SecondBase.Z += Height * 1.5f;

	FirstBase.X += Height * 0.5f;
	SecondBase.X -= Height * 0.5f;

	if (NodeArray[SwapNode1]->GrabbedComponent)
	{
		NodeArray[SwapNode1]->GrabbedComponent->SetWorldLocation(FirstBase);
	}
	if (NodeArray[SwapNode2]->GrabbedComponent)
	{
		NodeArray[SwapNode2]->GrabbedComponent->SetWorldLocation(SecondBase);
	}
}

void AA_StackEffect::TimelineFinished()
{
	UPrimitiveComponent* TempComponent = NodeArray[SwapNode1]->GrabbedComponent;
	NodeArray[SwapNode1]->GrabbedComponent = NodeArray[SwapNode2]->GrabbedComponent;
	NodeArray[SwapNode2]->GrabbedComponent = TempComponent;

	AActor* TempActor = NodeArray[SwapNode1]->GrabbedActor;
	NodeArray[SwapNode1]->GrabbedActor = NodeArray[SwapNode2]->GrabbedActor;
	NodeArray[SwapNode2]->GrabbedActor = TempActor;

	if (NodeArray[SwapNode1]->GrabbedComponent)
	{
		NodeArray[SwapNode1]->AttachToNode(NodeArray[SwapNode1]->GrabbedActor);
	}
	if (NodeArray[SwapNode2]->GrabbedComponent)
	{
		NodeArray[SwapNode2]->AttachToNode(NodeArray[SwapNode2]->GrabbedActor);
	}

	NodeArray[SwapNode1]->SetBorderMaterialIfOccupied(NodeArray[SwapNode1]->GrabbedComponent);
	NodeArray[SwapNode2]->SetBorderMaterialIfOccupied(NodeArray[SwapNode2]->GrabbedComponent);

	bIsSwapping = false;
	SwapAudioComp->Stop();
}