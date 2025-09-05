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

	//EndNode =  Cast<AA_ArrayNode>(EndNodeComponent->GetChildActor());
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

	int32 OutIndex1 = -1;

	int32 OutIndex2 = -1;

	int32 SizeOfConcatenatedArray = -1;

	bool bSplitDirecton = false;

	FText PrevName;

	FText NewName;

	int32 ArrayNum;

	//append
	if (ParseArrayIndexToAppend(Command))
	{
		AppendNode();
	}

	//swap
	else if (ParseArrayIndexToSwap(Command, OutIndex1, OutIndex2))
	{
		SwapNodes(OutIndex1, OutIndex2);
	}

	//delete
	else if (ParseArrayIndexToDel(Command, OutIndex1))
	{
		DeleteNode(OutIndex1);
	}

	//insert
	else if (ParseArrayIndexToInsert(Command, OutIndex1))
	{
		InsertNode(OutIndex1);
	}

	//pop
	else if (ParseArrayIndexToPop(Command))
	{
		ArrayPop();
	}

	//clear
	else if (ParseArrayIndexToClear(Command))
	{
		ArrayClear();
	}

	//extend
	else if (ParseArrayIndexToExtend(Command, ExtendArray))
	{
	}

	//concatenate
	else if (ParseArrayIndexToConcatenate(Command, SizeOfConcatenatingArray, SizeOfConcatenatedArray))
	{
		if (NodeArray.Num() != SizeOfConcatenatedArray)
		{
			return;
		}

		bIsOnConcatenation = true;
	}
	//reset concatenation
	else if (Command.ToString() == "reset()" && bIsOnConcatenation)
	{
		bIsOnConcatenation = false;
		bIsDetaching = true;;
	}
	//split
	else if (ParseArrayIndexToSplit(Command, OutIndex1, bSplitDirecton))
	{
		ArraySplit(OutIndex1, bSplitDirecton);
	}
	//rename
	else if (ParseNewNameToRename(Command, PrevName, NewName, ArrayNum) && PrevName.ToString() == ArrayName.ToString() && ArrayNum == NodeArray.Num())
	{
		ArrayRename(NewName);
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
		NodeArray[SwapNode1]->DetachComponent(NodeArray[SwapNode1]->GrabbedComponent);
	}

	if (NodeArray[SwapNode2]->GrabbedComponent)
	{
		NodeArray[SwapNode2]->DetachComponent(NodeArray[SwapNode2]->GrabbedComponent);
	}

	SwapAudioComp->SetWorldLocation(NodeArray[SwapNode1]->GetActorLocation());
	bIsSwapping = true;
	SwapAudioComp->Play();
	SwapTimeline->PlayFromStart();
}

void AA_ArrayEffect::DeleteNode(int32 Index)
{
	if (NodeArray.IsEmpty() || Index < 0 || Index >= NodeArray.Num())
	{
		return;
	}

	NodeArray[Index]->DeleteNode();

	for (size_t i = Index + 1; i < NodeArray.Num(); ++i)
	{
		NodeArray[i]->MoveNode(NodeArray[i]->DefaultLocation + GetActorRightVector() * NodeWidth);
		NodeArray[i]->SetIndex(i - 1);
	}

	NodeArray.RemoveAt(Index);
	EndNode->MoveNode(GetActorLocation() - GetActorRightVector() * NodeWidth * NodeArray.Num());
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

void AA_ArrayEffect::ArraySplit(int32 SplitIndex, bool MoveDirection)
{
	if (!ArrayClass)
	{
		return;
	}

	if (AA_ArrayEffect* NewArray = GetWorld()->SpawnActor<AA_ArrayEffect>(ArrayClass, NodeArray[SplitIndex]->DefaultLocation, GetActorRotation()))
	{
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
			NewArray->MoveArrayOnSplit(NewArray, MoveDirection);
		}
		else
		{
			MoveArrayOnSplit(this, MoveDirection);
		}
	}
}

void AA_ArrayEffect::ArrayRename(FText NewName)
{
	ArrayName = NewName;
	TextComp->SetText(ArrayName);
}

bool AA_ArrayEffect::ParseArrayIndexToAppend(FText Command)
{
	FString Input = Command.ToString();

	if (Input == "append()")
	{
		return true;
	}

	return false;
}

bool AA_ArrayEffect::ParseArrayIndexToSwap(FText Command, int32& OutIndex1, int32& OutIndex2)
{
	FString CleanExpr = Command.ToString().Replace(TEXT(" "), TEXT(""));

	FString LeftPart, RightPart;
	if (!CleanExpr.Split(TEXT("="), &LeftPart, &RightPart))
	{
		return false;
	}

	TArray<int32> LeftIndices;
	TArray<int32> RightIndices;

	auto ParseIndices = [](const FString& Part, TArray<int32>& OutIndices) -> bool
		{
			TArray<FString> Elements;
			Part.ParseIntoArray(Elements, TEXT(","), true);

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

				if (Name != TEXT("arr"))
					return false;

				int32 Index = FCString::Atoi(*IndexStr);
				OutIndices.Add(Index);
			}
			return OutIndices.Num() == 2;
		};


	if (ParseIndices(LeftPart, LeftIndices) && ParseIndices(RightPart, RightIndices) && LeftIndices[0] == RightIndices[1] && LeftIndices[1] == RightIndices[0])
	{
		OutIndex1 = LeftIndices[0];
		OutIndex2 = LeftIndices[1];

		return true;
	}

	return false;
}

bool AA_ArrayEffect::ParseArrayIndexToDel(FText Command, int32& OutIndex)
{
	FString Input = Command.ToString();

	int32 OpenBracket = 0;
	int32 CloseBracket = 0;

	if (!Input.FindChar('[', OpenBracket) || !Input.FindChar(']', CloseBracket) || OpenBracket >= CloseBracket || CloseBracket != Input.Len() - 1 || Input.Left(OpenBracket) != "del") return false;

	FString IndexStr = Input.Mid(OpenBracket + 1, CloseBracket - OpenBracket - 1);

	if (!IndexStr.IsNumeric()) return false;

	OutIndex = FCString::Atoi(*IndexStr);

	return true;
}

bool AA_ArrayEffect::ParseArrayIndexToInsert(FText Command, int32& OutIndex)
{
	FString Input = Command.ToString();

	int32 OpenBracket = 0;
	int32 CloseBracket = 0;

	if (!Input.FindChar('[', OpenBracket) || !Input.FindChar(']', CloseBracket) || OpenBracket >= CloseBracket || CloseBracket != Input.Len() - 1 || Input.Left(OpenBracket) != "insert") return false;

	FString IndexStr = Input.Mid(OpenBracket + 1, CloseBracket - OpenBracket - 1);

	if (!IndexStr.IsNumeric()) return false;

	OutIndex = FCString::Atoi(*IndexStr);

	return true;
}

bool AA_ArrayEffect::ParseArrayIndexToPop(FText Command)
{
	FString Input = Command.ToString();

	if (Input == "pop()")
	{
		return true;
	}

	return false;
}

bool AA_ArrayEffect::ParseArrayIndexToClear(FText Command)
{
	FString Input = Command.ToString();

	if (Input == "clear()")
	{
		return true;
	}

	return false;
}

bool AA_ArrayEffect::ParseArrayIndexToExtend(FText Command, TArray<int32>& OutArray)
{
	OutArray.Empty();
	FString Input = Command.ToString();

	if (!Input.StartsWith(TEXT("extend(")) || !Input.EndsWith(TEXT(")")))
	{
		return false;
	}

	FString Inner = Input.Mid(7, Input.Len() - 8);
	Inner.RemoveSpacesInline();

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

bool AA_ArrayEffect::ParseArrayIndexToConcatenate(FText Command, int32& OutSize1, int32& OutSize2)
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

bool AA_ArrayEffect::ParseArrayIndexToSplit(FText Command, int32& OutIndex, bool& Direction)
{
	const FString Input = Command.ToString();

	if (!Input.StartsWith(TEXT("arr[")) || !Input.EndsWith(TEXT("]")))
	{
		return false;
	}

	FString Inner = Input.Mid(4, Input.Len() - 5).TrimStartAndEnd();

	int32 ColonPos;
	if (!Inner.FindChar(TEXT(':'), ColonPos))
	{
		return false;
	}

	FString Left = Inner.Left(ColonPos).TrimStartAndEnd();
	FString Right = Inner.Mid(ColonPos + 1).TrimStartAndEnd();

	auto IsNumericPositive = [](const FString& Str) -> bool
		{
			return !Str.IsEmpty() && Str.IsNumeric();
		};

	if (Left.IsEmpty() && IsNumericPositive(Right))
	{
		OutIndex = FCString::Atoi(*Right);
		Direction = false;
		return true;
	}
	else if (Right.IsEmpty() && IsNumericPositive(Left))
	{
		OutIndex = FCString::Atoi(*Left);
		Direction = true;
		return true;
	}

	return false;
}

bool AA_ArrayEffect::ParseNewNameToRename(FText Command, FText& PrevName, FText& NewName, int32& ArrayNum)
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
	const FString ParsedName = Left;
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
	if (!Lines[1].Split(TEXT("="), &Left, &Right) || Left.IsEmpty() || Right.IsEmpty() || Right != ParsedName)
	{
		return false;
	}
	NewName = FText::FromString(Left);

	if (!Lines[2].StartsWith(TEXT("del ")))
	{
		return false;
	}
	FString DelName = Lines[2].Mid(4).TrimStartAndEnd();
	if (DelName.IsEmpty() || DelName != ParsedName)
	{
		return false;
	}
	PrevName = FText::FromString(DelName);

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

void AA_ArrayEffect::MoveArrayOnSplit(AA_ArrayEffect* ArrayToMove, bool Direction)
{
	int32 MoveDirection = Direction * 2 - 1;

	ArrayToMove->SetActorLocation(ArrayToMove->GetActorLocation() - ArrayToMove->GetActorRightVector() * 2 * ArrayToMove->NodeWidth * MoveDirection);
	ArrayToMove->DefaultLocation = GetActorLocation();

	for (AA_ArrayNode* Node : ArrayToMove->NodeArray)
	{
		Node->MoveNode(Node->DefaultLocation - ArrayToMove->GetActorRightVector() * 2 * ArrayToMove->NodeWidth * MoveDirection);
	}

	ArrayToMove->EndNode->MoveNode(ArrayToMove->EndNode->DefaultLocation - ArrayToMove->GetActorRightVector() * 2 * ArrayToMove->NodeWidth * MoveDirection);
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
	FVector LineDir = (End - Start).GetSafeNormal();
	float Dot = FVector::DotProduct(Player->GetActorLocation() - Start, LineDir);
	float ClampedDist = FMath::Clamp(Dot, 0.f, FVector::Dist(Start, End));
	FVector ClampedPos = Start + LineDir * ClampedDist;
	FVector PosToPlayerDir = Player->GetActorLocation() - ClampedPos;
	PosToPlayerDir.Normalize();
	ClampedPos += PosToPlayerDir * 100.0f;
	ClampedPos.Z = GetActorLocation().Z - 140.0f + FMath::Sin(GetWorld()->GetTimeSeconds()) * 10.0f;
	FVector NameLocation = FMath::VInterpTo(TextComp->GetComponentLocation(), ClampedPos, GetWorld()->GetDeltaSeconds(), 2.0f);
	TextComp->SetWorldLocation(NameLocation);

	FRotator NameRotation = (Player->GetActorLocation() - TextComp->GetComponentLocation()).Rotation();
	TextComp->SetWorldRotation(NameRotation);
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

	if (NodeArray[SwapNode1]->GrabbedComponent)
	{
		NodeArray[SwapNode1]->AttachComponent(NodeArray[SwapNode1]->GrabbedComponent);
	}
	if (NodeArray[SwapNode2]->GrabbedComponent)
	{
		NodeArray[SwapNode2]->AttachComponent(NodeArray[SwapNode2]->GrabbedComponent);
	}

	NodeArray[SwapNode1]->SetBorderMaterialIfOccupied(NodeArray[SwapNode1]->GrabbedComponent);
	NodeArray[SwapNode2]->SetBorderMaterialIfOccupied(NodeArray[SwapNode2]->GrabbedComponent);

	bIsSwapping = false;
	SwapAudioComp->Stop();
}