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
	AppendNode->SetIndex(ArrayIndex++);
	FVector SpawnLocation = AppendNode->GetActorLocation();
	AA_ArrayNode* NewNode = AppendNode;
	NodeArray.Add(NewNode);

	AppendNode = GetWorld()->SpawnActor<AA_ArrayNode>(NodeClass, SpawnLocation, FRotator::ZeroRotator);
	if (AppendNode)
	{
		AppendNode->OnGrabDel.AddDynamic(this, &AA_ArrayEffect::AddNewNode);
		AppendNode->OnDeleteDel.AddDynamic(this, &AA_ArrayEffect::DeleteNode);

		AppendNode->MoveNode(false);
	}
}

void AA_ArrayEffect::DeleteNode(int32 Index)
{
	//NodeArray.RemoveAt(Index);
	//AppendNode->bIsMoving = true;
	//bIsDeletingNode = true;
}