#include "ArrayEffect/A_ArrayEffect.h"
#include "ArrayEffect/A_ArrayNode.h"

AA_ArrayEffect::AA_ArrayEffect()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	AppendNodeComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("AppendNode"));

	RootComponent = SceneComponent;
	AppendNodeComponent->SetupAttachment(RootComponent);
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

	AppendNode->MoveNode(*--LocationArray.end());

	NodeArray.RemoveAt(Index);
	LocationArray.Pop();
}

void AA_ArrayEffect::SwapNode(int32 Node1, int32 Node2)
{
	if (Node1 < 0 || Node1 > NodeArray.Num() - 1 || Node2 < 0 || Node2 > NodeArray.Num() - 1 || Node1 == Node2)
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("%d  %d"), Node1, Node2));
}
