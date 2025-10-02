#include "ArrayEffect/A_ArrayEffect.h"
#include "ArrayEffect/A_ContainerNode.h"
#include "Components/AudioComponent.h"

AA_ArrayEffect::AA_ArrayEffect()
{
	PrimaryActorTick.bCanEverTick = true;

	SwapTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SwapTimeline"));
	SwapAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("SwapAudioComponent"));

	SwapAudioComp->SetupAttachment(RootComponent);

	SwapAudioComp->bAutoActivate = false;
}

void AA_ArrayEffect::BeginPlay()
{
	Super::BeginPlay();

	if (FloatCurve)
	{
		ProgressFunction.BindUFunction(this, FName("SwapTimelineProgress"));
		SwapTimeline->AddInterpFloat(FloatCurve, ProgressFunction);

		FinishedFunction.BindUFunction(this, FName("SwapTimelineFinished"));
		SwapTimeline->SetTimelineFinishedFunc(FinishedFunction);

		SwapTimeline->SetLooping(false);
	}
}

void AA_ArrayEffect::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_ArrayEffect::GetTextCommand(FText Command)
{
	Super::GetTextCommand(Command);

	int32 OutIndex = -1;

	int32 OutLeftIndex = -1;

	int32 OutRightIndex = -1;

	FText PrevName;

	FName VariableName = NAME_None;

	//swap
	if (ParseCommandToSwap(Command, PrevName, OutLeftIndex, OutRightIndex) && PrevName.ToString() == ContainerName.ToString())
	{
		SwapNodes(OutLeftIndex, OutRightIndex);
	}
	//insert
	else if (ParseCommandToInsert(Command, PrevName, VariableName, OutIndex) && PrevName.ToString() == ContainerName.ToString())
	{
		InsertNode(VariableName, OutIndex);
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

void AA_ArrayEffect::InsertNode(FName VariableName, int32 Index)
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

	AA_ContainerNode* NewNode = GetWorld()->SpawnActor<AA_ContainerNode>(NodeClass, SpawnLocation, GetActorRotation());
	if (NewNode)
	{
		NewNode->OwnerActor = this;
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
			return OutIndices.Num() == 2;
		};

	if (ParseIndices(Left, LeftIndices) && ParseIndices(Right, RightIndices) && LeftIndices[0] == RightIndices[1] && LeftIndices[1] == RightIndices[0])
	{
		OutIndex1 = LeftIndices[0];
		OutIndex2 = LeftIndices[1];

		return true;
	}

	return false;
}

bool AA_ArrayEffect::ParseCommandToInsert(FText Command, FText& PrevName, FName& VariableName, int32& OutIndex)
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

	if (!IsValidPythonIdentifier(ParsedArrayName))
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

void AA_ArrayEffect::SwapTimelineProgress(float Value)
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

void AA_ArrayEffect::SwapTimelineFinished()
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