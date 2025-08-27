#include "ArrayEffect/A_ArrayEffect.h"
#include "ArrayEffect/A_ArrayNode.h"
#include "Components/AudioComponent.h"

AA_ArrayEffect::AA_ArrayEffect()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	AppendNodeComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("AppendNode"));
	SwapTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("MoveTimeline"));
	SwapAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("SwapAudioComponent"));

	RootComponent = SceneComponent;
	AppendNodeComponent->SetupAttachment(RootComponent);
	SwapAudioComp->SetupAttachment(RootComponent);
	SwapAudioComp->bAutoActivate = false;
}

void AA_ArrayEffect::BeginPlay()
{
	Super::BeginPlay();


	AppendNode = Cast<AA_ArrayNode>(AppendNodeComponent->GetChildActor());
	if (AppendNode)
	{
		AppendNode->OwnerActor = this;
		AppendNode->OnGrabDel.AddDynamic(this, &AA_ArrayEffect::AddNewNode);
		AppendNode->OnDeleteDel.AddDynamic(this, &AA_ArrayEffect::DeleteNode);
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

}

void AA_ArrayEffect::AddNewNode()
{
	AppendNode->SetIndex(NodeArray.Num());
	LocationArray.Add(AppendNode->GetActorLocation());
	FVector SpawnLocation = AppendNode->GetActorLocation();
	AA_ArrayNode* NewNode = AppendNode;
	NodeArray.Add(NewNode);

	AppendNode = GetWorld()->SpawnActor<AA_ArrayNode>(NodeClass, SpawnLocation, FRotator::ZeroRotator);
	if (AppendNode)
	{
		AppendNode->OwnerActor = this;
		AppendNode->OnGrabDel.AddDynamic(this, &AA_ArrayEffect::AddNewNode);
		AppendNode->OnDeleteDel.AddDynamic(this, &AA_ArrayEffect::DeleteNode);

		AppendNode->MoveNode(AppendNode->GetActorLocation() - GetActorRightVector() * AppendNode->NodeBorder->Bounds.BoxExtent.Y * 2);
	}
}

void AA_ArrayEffect::DeleteNode(int32 Index)
{
	for (size_t i = Index + 1; i < NodeArray.Num(); ++i)
	{
		NodeArray[i]->MoveNode(LocationArray[i - 1]);
		NodeArray[i]->SetIndex(i - 1);
	}

	if (LocationArray.Num() > 0)
	{
		AppendNode->MoveNode(LocationArray.Last());
	}

	NodeArray.RemoveAt(Index);
	LocationArray.Pop();
}

void AA_ArrayEffect::SwapNode(int32 Node1, int32 Node2)
{
	if (Node1 < 0 || Node1 > NodeArray.Num() - 1 || Node2 < 0 || Node2 > NodeArray.Num() - 1 || Node1 == Node2)
	{
		return;
	}

	SwapNode1 = Node1;
	SwapNode2 = Node2;

	NodeArray[SwapNode1]->DetachComponent(NodeArray[SwapNode1]->GrabbedComponent);
	NodeArray[SwapNode2]->DetachComponent(NodeArray[SwapNode2]->GrabbedComponent);

	SwapAudioComp->SetWorldLocation(NodeArray[SwapNode1]->GetActorLocation());
	bIsSwapping = true;
	SwapAudioComp->Play();
	SwapTimeline->PlayFromStart();
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

	NodeArray[SwapNode1]->AttachComponent(NodeArray[SwapNode1]->GrabbedComponent);
	NodeArray[SwapNode2]->AttachComponent(NodeArray[SwapNode2]->GrabbedComponent);

	bIsSwapping = false;
	SwapAudioComp->Stop();
}