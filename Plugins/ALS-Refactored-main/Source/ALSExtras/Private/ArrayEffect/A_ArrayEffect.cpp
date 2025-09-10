#include "ArrayEffect/A_ArrayEffect.h"
#include "ArrayEffect/A_ArrayNode.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AlsCharacterExample.h"
#include "AlsCameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"

AA_ArrayEffect::AA_ArrayEffect()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollisionComponent"));
	EndNodeComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("EndNodeComponent"));
	SwapTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SwapTimeline"));
	SwapAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("SwapAudioComponent"));
	TextComp = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRenderComponent"));

	RootComponent = SceneComponent;
	CollisionComponent->SetupAttachment(RootComponent);
	EndNodeComponent->SetupAttachment(RootComponent);
	SwapAudioComp->SetupAttachment(RootComponent);
	TextComp->SetupAttachment(RootComponent);

	SwapAudioComp->bAutoActivate = false;
}

void AA_ArrayEffect::BeginPlay()
{
	Super::BeginPlay();

	DefaultLocation = GetActorLocation();
	DefaultRotation = GetActorRotation();

	EndNodeComponent->DestroyComponent();
	EndNode = GetWorld()->SpawnActor<AA_ArrayNode>(NodeClass, GetActorLocation(), GetActorRotation());
	if (EndNode)
	{
		EndNode->OwnerActor = this;

		FVector MinBound;
		FVector MaxBound;
		EndNode->NodeBorder->GetLocalBounds(MinBound, MaxBound);
		MinBound *= EndNode->NodeBorder->GetComponentScale();
		MaxBound *= EndNode->NodeBorder->GetComponentScale();
		NodeLength = MaxBound.X - MinBound.X;
		NodeWidth = MaxBound.Y - MinBound.Y;
		NodeHigh = MaxBound.Z - MinBound.Z;
	}

	CollisionComponent->SetBoxExtent(FVector(NodeLength * 0.5f, NodeWidth * 0.5f, NodeHigh * 0.5f));

	if (FloatCurve)
	{
		ProgressFunction.BindUFunction(this, FName("TimelineProgress"));
		SwapTimeline->AddInterpFloat(FloatCurve, ProgressFunction);

		FinishedFunction.BindUFunction(this, FName("TimelineFinished"));
		SwapTimeline->SetTimelineFinishedFunc(FinishedFunction);

		SwapTimeline->SetLooping(false);
	}

	TextComp->SetText(ArrayName);
}

void AA_ArrayEffect::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ArrayExtend();

	AttachToCharacterCamera();

	DetachFromCharacterCamera();

	AttachToArray();

	RefreshNameLocationAndRotation();
}

void AA_ArrayEffect::GetTextCommand(FText Command)
{
	if (bIsSwapping || EndNode->bIsMoving || bIsDetaching)
	{
		return;
	}

	int32 OutIndex = -1;

	int32 OutLeftIndex = -1;

	int32 OutRightIndex = -1;

	int32 SizeOfConcatenatedArray = -1;

	bool bSplitDirecton = false;

	FText PrevName;

	FText NewName;

	FText CopyName;

	int32 ArrayNum;

	//append
	if (ParseCommandToAppend(Command, PrevName) && PrevName.ToString() == ArrayName.ToString())
	{
		AppendNode();
	}

	//swap
	else if (ParseCommandToSwap(Command, PrevName, OutLeftIndex, OutRightIndex) && PrevName.ToString() == ArrayName.ToString())
	{
		SwapNodes(OutLeftIndex, OutRightIndex);
	}

	//delete
	else if (ParseCommandToDelete(Command, PrevName, OutIndex, OutLeftIndex, OutRightIndex) && PrevName.ToString() == ArrayName.ToString())
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
			ArrayClear();
			EndNode->Destroy();
			Destroy();
		}
		MoveNodesConsideringOrder();
	}

	//insert
	else if (ParseCommandToInsert(Command, PrevName, OutIndex) && PrevName.ToString() == ArrayName.ToString())
	{
		InsertNode(OutIndex);
	}

	//pop
	else if (ParseCommandToPop(Command, PrevName) && PrevName.ToString() == ArrayName.ToString())
	{
		ArrayPop();
	}

	//clear
	else if (ParseCommandToClear(Command, PrevName) && PrevName.ToString() == ArrayName.ToString())
	{
		ArrayClear();
		MoveNodesConsideringOrder();
	}

	//extend
	else if (ParseCommandToExtend(Command, PrevName, ExtendArray) && PrevName.ToString() == ArrayName.ToString())
	{
	}

	//concatenate
	else if (ParseCommandToConcatenate(Command, SizeOfConcatenatingArray, SizeOfConcatenatedArray))
	{
		if (NodeArray.Num() != SizeOfConcatenatedArray)
		{
			return;
		}

		bIsOnConcatenation = true;
	}
	//reset concatenation
	else if (bIsOnConcatenation && ParseCommandToReset(Command, PrevName) && PrevName.ToString() == ArrayName.ToString())
	{
		bIsOnConcatenation = false;
		bIsDetaching = true;;
	}
	//split
	else if (ParseCommandToSplit(Command, PrevName, NewName, OutIndex, bSplitDirecton) && PrevName.ToString() == ArrayName.ToString())
	{
		ArrayCopy(NewName, OutIndex, bSplitDirecton);
	}
	//rename
	else if (ParseCommandToRename(Command, PrevName, NewName, ArrayNum) && PrevName.ToString() == ArrayName.ToString() && ArrayNum == NodeArray.Num())
	{
		ArrayRename(NewName);
	}
	//copy
	else if (ParseCommandToCopy(Command, PrevName, CopyName) && PrevName.ToString() == ArrayName.ToString())
	{
		ArrayCopy(CopyName);
	}
}

void AA_ArrayEffect::AppendNode()
{
	if (NodeArray.Num() == 10)
	{
		return;
	}

	EndNode->SetIndex(NodeArray.Num());
	FVector SpawnLocation = EndNode->GetActorLocation();
	AA_ArrayNode* NewNode = EndNode;
	NodeArray.Add(NewNode);
	if (NodeClass)
	{
		EndNode = GetWorld()->SpawnActor<AA_ArrayNode>(NodeClass, SpawnLocation, GetActorRotation());
		if (EndNode)
		{
			EndNode->OwnerActor = this;
			EndNode->MoveNode(EndNode->GetActorLocation() - GetActorRightVector() * NodeWidth);
		}
	}
}

void AA_ArrayEffect::SwapNodes(int32 Node1, int32 Node2)
{
	if (Node1 < 0 || Node1 > NodeArray.Num() - 1 || Node2 < 0 || Node2 > NodeArray.Num() - 1 || Node1 == Node2)
	{
		return;
	}

	SwapNode1 = Node1;
	SwapNode2 = Node2;

	if (NodeArray[SwapNode1]->GrabbedComponent)
	{
		NodeArray[SwapNode1]->DetachFromNode(NodeArray[SwapNode1]->GrabbedActor);
	}

	if (NodeArray[SwapNode2]->GrabbedComponent)
	{
		NodeArray[SwapNode2]->DetachFromNode(NodeArray[SwapNode2]->GrabbedActor);
	}

	SwapAudioComp->SetWorldLocation(NodeArray[SwapNode1]->GetActorLocation());
	bIsSwapping = true;
	SwapAudioComp->Play();
	SwapTimeline->PlayFromStart();
}

void AA_ArrayEffect::DeleteNode(int32 Index)
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

void AA_ArrayEffect::InsertNode(int32 Index)
{
	if (Index < 0 || Index >= NodeArray.Num() || NodeArray.Num() == 10)
	{
		return;
	}

	EndNode->MoveNode(EndNode->GetActorLocation() - GetActorRightVector() * NodeWidth);
	FVector SpawnLocation = NodeArray[Index]->GetActorLocation();

	for (size_t i = Index; i < NodeArray.Num(); ++i)
	{
		NodeArray[i]->MoveNode(NodeArray[i]->GetActorLocation() - GetActorRightVector() * NodeWidth);
		NodeArray[i]->SetIndex(i + 1);
	}

	AA_ArrayNode* NewNode = GetWorld()->SpawnActor<AA_ArrayNode>(NodeClass, SpawnLocation, GetActorRotation());
	if (NewNode)
	{
		NewNode->OwnerActor = this;
		NewNode->SetIndex(Index);
		NodeArray.Insert(NewNode, Index);
	}
}

void AA_ArrayEffect::ArrayPop()
{
	DeleteNode(NodeArray.Num() - 1);
}

void AA_ArrayEffect::ArrayClear()
{
	for (int32 i = NodeArray.Num() - 1; i >= 0; --i)
	{
		DeleteNode(i);
	}
}

void AA_ArrayEffect::ArrayExtend()
{
	if (EndNode->bIsMoving)
	{
		return;
	}

	if (ExtendArrayIndex < ExtendArray.Num())
	{
		++ExtendArrayIndex;
		AppendNode();
	}
	else
	{
		ExtendArray.Empty();
		ExtendArrayIndex = 0;
	}
}

void AA_ArrayEffect::ArrayConcatenate(AA_ArrayEffect* ArrayToConcatenate)
{
	if (NodeArray.Num() != ArrayToConcatenate->SizeOfConcatenatingArray)
	{
		ArrayToConcatenate->bIsOnConcatenation = false;
		ArrayToConcatenate->bIsDetaching = true;
		return;
	}

	ArrayToConcatenate->bIsOnConcatenation = false;

	int32 Index = NodeArray.Num();
	for (AA_ArrayNode* Node : ArrayToConcatenate->NodeArray)
	{
		Node->OwnerActor = this;
		Node->DefaultLocation = GetActorLocation() - GetActorRightVector() * NodeWidth * Index;
		Node->SetIndex(Index++);
	}

	ArrayToConcatenate->EndNode->Destroy();
	NodeArray.Append(ArrayToConcatenate->NodeArray);
	EndNode->DefaultLocation = NodeArray.Last()->DefaultLocation - GetActorRightVector() * NodeWidth;
	ArrayToConcatenate->Destroy();

	bIsAttaching = true;
}

void AA_ArrayEffect::ArraySplit(int32 SplitIndex, bool MoveDirection, FText& NewName)
{
	if (!ArrayClass)
	{
		return;
	}

	if (AA_ArrayEffect* NewArray = GetWorld()->SpawnActor<AA_ArrayEffect>(ArrayClass, NodeArray[SplitIndex]->DefaultLocation, GetActorRotation()))
	{
		NewArray->SetArrayName(NewName);
		AA_ArrayNode* TempNode = NewArray->EndNode;
		NewArray->EndNode = EndNode;
		NewArray->EndNode->OwnerActor = NewArray;

		EndNode = TempNode;
		EndNode->OwnerActor = this;

		NewArray->NodeArray.Append(NodeArray.GetData() + SplitIndex, NodeArray.Num() - SplitIndex);
		int32 NewIndex = 0;
		for (AA_ArrayNode* Node : NewArray->NodeArray)
		{
			Node->OwnerActor = NewArray;
			Node->SetIndex(NewIndex++);
		}

		NodeArray.SetNum(SplitIndex);

		if (MoveDirection)
		{
			NewArray->MoveArray(NewArray->GetActorLocation() - NewArray->GetActorRightVector() * NewArray->NodeWidth * 2);
		}
		else
		{
			MoveArray(GetActorLocation() + GetActorRightVector() * NodeWidth * 2);
		}
	}
}

void AA_ArrayEffect::ArrayRename(FText NewName)
{
	SetArrayName(NewName);
}

void AA_ArrayEffect::ArrayCopy(FText Name, int32 SplitIndex, bool MoveDirection)
{
	if (!ArrayClass)
	{
		return;
	}

	int32 LeftIndex = 0;
	int32 RightIndex = NodeArray.Num();

	if (SplitIndex != -1)
	{
		if (MoveDirection)
		{
			LeftIndex = SplitIndex;
		}
		else
		{
			RightIndex = SplitIndex;
		}
	}

	if (AA_ArrayEffect* NewArray = GetWorld()->SpawnActor<AA_ArrayEffect>(ArrayClass, EndNode->DefaultLocation, GetActorRotation()))
	{
		NewArray->SetArrayName(Name);

		int32 Index = 0;
		for (size_t i = LeftIndex; i < RightIndex; ++i)
		{
			if (AA_ArrayNode* NewNode = GetWorld()->SpawnActor<AA_ArrayNode>(NodeClass, NodeArray[i]->GetActorLocation(), GetActorRotation()))
			{
				NewNode->OwnerActor = NewArray;
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
		NewArray->MoveArray(EndNode->DefaultLocation - GetActorRightVector() * NodeWidth * 2);
	}
}

bool AA_ArrayEffect::IsValidPythonIdentifier(const FString& Str)
{
	if (Str.IsEmpty())
	{
		return false;
	}

	if (FChar::IsDigit(Str[0]))
	{
		return false;
	}

	for (TCHAR Ch : Str)
	{
		if (!(FChar::IsAlpha(Ch) || FChar::IsDigit(Ch) || Ch == TEXT('_')))
		{
			return false;
		}
	}

	static const TSet<FString> PythonKeywords =
	{
		TEXT("False"), TEXT("None"), TEXT("True"), TEXT("and"), TEXT("as"),
		TEXT("assert"), TEXT("break"), TEXT("class"), TEXT("continue"),
		TEXT("def"), TEXT("del"), TEXT("elif"), TEXT("else"), TEXT("except"),
		TEXT("finally"), TEXT("for"), TEXT("from"), TEXT("global"), TEXT("if"),
		TEXT("import"), TEXT("in"), TEXT("is"), TEXT("lambda"), TEXT("nonlocal"),
		TEXT("not"), TEXT("or"), TEXT("pass"), TEXT("raise"), TEXT("return"),
		TEXT("try"), TEXT("while"), TEXT("with"), TEXT("yield"),
		TEXT("list"), TEXT("dict"), TEXT("set"), TEXT("int"), TEXT("str")
	};

	if (PythonKeywords.Contains(Str))
	{
		return false;
	}

	return true;
}

bool AA_ArrayEffect::ParseCommandToAppend(FText Command, FText& PrevName)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	if (!Input.EndsWith(TEXT(".append()")))
	{
		return false;
	}

	FString ParsedArrayName = Input.LeftChop(9);

	if (!IsValidPythonIdentifier(ParsedArrayName))
	{
		return false;
	}

	PrevName = FText::FromString(ParsedArrayName);
	return true;
}

bool AA_ArrayEffect::ParseCommandToSwap(FText Command, FText& PrevName, int32& OutIndex1, int32& OutIndex2)
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
					if (IsValidPythonIdentifier(Name))
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

bool AA_ArrayEffect::ParseCommandToDelete(FText Command, FText& PrevName, int32& OutIndex, int32& OutLeftIndex, int32& OutRightIndex)
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
		if (!IsValidPythonIdentifier(Remainder))
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
	if (!IsValidPythonIdentifier(ParsedArrayName))
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
	if (Left.IsEmpty() && Right.IsNumeric()) // [:index]
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

bool AA_ArrayEffect::ParseCommandToInsert(FText Command, FText& PrevName, int32& OutIndex)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	const FString InsertSuffix = TEXT(".insert[");

	int32 InsertPos = Input.Find(InsertSuffix);
	if (InsertPos == INDEX_NONE)
	{
		return false;
	}

	FString ParsedArrayName = Input.Left(InsertPos);
	if (!IsValidPythonIdentifier(ParsedArrayName))
	{
		return false;
	}

	PrevName = FText::FromString(ParsedArrayName);

	int32 OpenBracket = InsertPos + InsertSuffix.Len() - 1;
	int32 CloseBracket = 0;

	if (!Input.FindLastChar(']', CloseBracket) || CloseBracket <= OpenBracket || CloseBracket != Input.Len() - 1)
	{
		return false;
	}

	FString IndexStr = Input.Mid(OpenBracket + 1, CloseBracket - OpenBracket - 1);

	if (!IndexStr.IsNumeric())
	{
		return false;
	}

	OutIndex = FCString::Atoi(*IndexStr);
	return true;
}

bool AA_ArrayEffect::ParseCommandToPop(FText Command, FText& PrevName)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	const FString Suffix = TEXT(".pop()");
	if (!Input.EndsWith(Suffix))
	{
		return false;
	}

	FString ParsedArrayName = Input.LeftChop(Suffix.Len());

	if (!IsValidPythonIdentifier(ParsedArrayName))
	{
		return false;
	}

	PrevName = FText::FromString(ParsedArrayName);
	return true;
}

bool AA_ArrayEffect::ParseCommandToClear(FText Command, FText& PrevName)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	const FString Suffix = TEXT(".clear()");
	if (!Input.EndsWith(Suffix))
	{
		return false;
	}

	FString ParsedArrayName = Input.LeftChop(Suffix.Len());

	if (!IsValidPythonIdentifier(ParsedArrayName))
	{
		return false;
	}

	PrevName = FText::FromString(ParsedArrayName);
	return true;
}

bool AA_ArrayEffect::ParseCommandToExtend(FText Command, FText& PrevName, TArray<int32>& OutArray)
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
	if (!IsValidPythonIdentifier(ParsedArrayName))
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

bool AA_ArrayEffect::ParseCommandToConcatenate(FText Command, int32& OutSize1, int32& OutSize2)
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

bool AA_ArrayEffect::ParseCommandToReset(FText Command, FText& PrevName)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	const FString Suffix = TEXT(".reset()");
	if (!Input.EndsWith(Suffix))
	{
		return false;
	}

	FString ParsedArrayName = Input.LeftChop(Suffix.Len());

	if (!IsValidPythonIdentifier(ParsedArrayName))
	{
		return false;
	}

	PrevName = FText::FromString(ParsedArrayName);
	return true;
}

bool AA_ArrayEffect::ParseCommandToSplit(FText Command, FText& PrevName, FText& NewName, int32& OutIndex, bool& Direction)
{
	FString Input = Command.ToString();
	Input.RemoveSpacesInline();

	int32 BracketOpen, BracketClose;
	if (!Input.FindChar('[', BracketOpen) || !Input.FindLastChar(']', BracketClose))
	{
		return false;
	}

	if (BracketOpen <= 0 || BracketClose != Input.Len() - 1)
	{
		return false;
	}

	FString ParsedArrayName = Input.Left(BracketOpen);
	if (!IsValidPythonIdentifier(ParsedArrayName))
	{
		return false;
	}

	PrevName = FText::FromString(ParsedArrayName);
	NewName = FText::FromString(ParsedArrayName + TEXT("_1"));

	FString Inner = Input.Mid(BracketOpen + 1, BracketClose - BracketOpen - 1);

	int32 ColonPos;
	if (!Inner.FindChar(TEXT(':'), ColonPos))
	{
		return false;
	}

	FString Left = Inner.Left(ColonPos);
	FString Right = Inner.Mid(ColonPos + 1);

	auto IsNumericPositive = [](const FString& Str) -> bool
		{
			return !Str.IsEmpty() && Str.IsNumeric();
		};

	if (Left.IsEmpty() && IsNumericPositive(Right)) // arrname[:N]
	{
		OutIndex = FCString::Atoi(*Right);
		Direction = false;
		return true;
	}
	else if (Right.IsEmpty() && IsNumericPositive(Left)) // arrname[N:]
	{
		OutIndex = FCString::Atoi(*Left);
		Direction = true;
		return true;
	}

	return false;
}


bool AA_ArrayEffect::ParseCommandToRename(FText Command, FText& PrevName, FText& NewName, int32& ArrayNum)
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
	if (!IsValidPythonIdentifier(Left))
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

bool AA_ArrayEffect::ParseCommandToCopy(FText Command, FText& PrevName, FText& CopyName)
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

	if (Right.EndsWith(TEXT("[:]")))
	{
		BaseName = Right.LeftChop(3);
	}
	else if (Right.EndsWith(TEXT(".copy()")))
	{
		BaseName = Right.LeftChop(7);
	}
	else if (Right.StartsWith(TEXT("list(")) && Right.EndsWith(TEXT(")")))
	{
		BaseName = Right.Mid(5, Right.Len() - 6);
	}
	else
	{
		return false;
	}

	if (!IsValidPythonIdentifier(BaseName) || !IsValidPythonIdentifier(Left))
	{
		return false;
	}

	if (BaseName == Left)
	{
		return false;
	}

	PrevName = FText::FromString(BaseName);
	CopyName = FText::FromString(Left);

	return true;
}

void AA_ArrayEffect::AttachToCharacterCamera()
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

void AA_ArrayEffect::DetachFromCharacterCamera()
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

void AA_ArrayEffect::AttachToArray()
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

void AA_ArrayEffect::MoveArray(FVector NewLocation)
{
	SetActorLocation(NewLocation);
	DefaultLocation = GetActorLocation();

	MoveNodesConsideringOrder();
}

void AA_ArrayEffect::MoveNodesConsideringOrder()
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

void AA_ArrayEffect::RefreshNameLocationAndRotation()
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

void AA_ArrayEffect::SetArrayName(FText Name)
{
	ArrayName = Name;
	TextComp->SetText(Name);
}

void AA_ArrayEffect::TimelineProgress(float Value)
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

void AA_ArrayEffect::TimelineFinished()
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