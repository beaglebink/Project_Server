#include "ArrayEffect/A_ArrayEffect.h"
#include "ArrayEffect/A_ArrayNode.h"
#include "Components/AudioComponent.h"

AA_ArrayEffect::AA_ArrayEffect()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	EndNodeComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("EndNodeComponent"));
	SwapTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("MoveTimeline"));
	SwapAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("SwapAudioComponent"));

	RootComponent = SceneComponent;
	EndNodeComponent->SetupAttachment(RootComponent);
	SwapAudioComp->SetupAttachment(RootComponent);
	SwapAudioComp->bAutoActivate = false;
}

void AA_ArrayEffect::BeginPlay()
{
	Super::BeginPlay();


	EndNode = Cast<AA_ArrayNode>(EndNodeComponent->GetChildActor());
	if (EndNode)
	{
		EndNode->OwnerActor = this;
		EndNode->OnGrabDel.AddDynamic(this, &AA_ArrayEffect::AppendNode);
	}

	if (FloatCurve)
	{
		ProgressFunction.BindUFunction(this, FName("TimelineProgress"));
		SwapTimeline->AddInterpFloat(FloatCurve, ProgressFunction);

		FinishedFunction.BindUFunction(this, FName("TimelineFinished"));
		SwapTimeline->SetTimelineFinishedFunc(FinishedFunction);

		SwapTimeline->SetLooping(false);
	}
}

void AA_ArrayEffect::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ArrayExtend();
}

void AA_ArrayEffect::GetTextCommand(FText Command)
{
	if (bIsSwapping || EndNode->bIsMoving)
	{
		return;
	}

	int32 OutIndex1 = -1;

	int32 OutIndex2 = -1;

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
}

void AA_ArrayEffect::AppendNode()
{
	if (NodeArray.Num() == 10)
	{
		return;
	}

	EndNode->SetIndex(NodeArray.Num());
	LocationArray.Add(EndNode->GetActorLocation());
	FVector SpawnLocation = EndNode->GetActorLocation();
	AA_ArrayNode* NewNode = EndNode;
	NodeArray.Add(NewNode);

	EndNode = GetWorld()->SpawnActor<AA_ArrayNode>(NodeClass, SpawnLocation, FRotator::ZeroRotator);
	if (EndNode)
	{
		EndNode->OwnerActor = this;
		EndNode->OnGrabDel.AddDynamic(this, &AA_ArrayEffect::AppendNode);

		EndNode->MoveNode(EndNode->GetActorLocation() - GetActorRightVector() * EndNode->NodeBorder->Bounds.BoxExtent.Y * 2);
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
		NodeArray[i]->MoveNode(LocationArray[i - 1]);
		NodeArray[i]->SetIndex(i - 1);
	}

	if (LocationArray.Num() > 0)
	{
		EndNode->MoveNode(LocationArray.Last());
	}

	NodeArray.RemoveAt(Index);
	LocationArray.Pop();
}

void AA_ArrayEffect::InsertNode(int32 Index)
{
	if (Index < 0 || Index >= NodeArray.Num() || NodeArray.Num() == 10)
	{
		return;
	}

	LocationArray.Add(EndNode->GetActorLocation());
	EndNode->MoveNode(EndNode->GetActorLocation() - GetActorRightVector() * EndNode->NodeBorder->Bounds.BoxExtent.Y * 2);
	FVector SpawnLocation = NodeArray[Index]->GetActorLocation();

	for (size_t i = Index; i < NodeArray.Num(); ++i)
	{
		NodeArray[i]->MoveNode(NodeArray[i]->GetActorLocation() - GetActorRightVector() * EndNode->NodeBorder->Bounds.BoxExtent.Y * 2);
		NodeArray[i]->SetIndex(i + 1);
	}

	AA_ArrayNode* NewNode = GetWorld()->SpawnActor<AA_ArrayNode>(NodeClass, SpawnLocation, FRotator::ZeroRotator);
	if (NewNode)
	{
		NewNode->OwnerActor = this;
		NewNode->SetIndex(Index);
		NodeArray.Insert(NewNode, Index);
		NewNode->OnGrabDel.AddDynamic(this, &AA_ArrayEffect::AppendNode);
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
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("%2d"), ExtendArray.Num()));

		++ExtendArrayIndex;
		AppendNode();
	}
	else
	{
		ExtendArray.Empty();
		ExtendArrayIndex = 0;
	}
}

void AA_ArrayEffect::ArrayConcatenate()
{
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

bool AA_ArrayEffect::ParseArrayIndexToConcatenate(FText Command)
{
	return false;
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