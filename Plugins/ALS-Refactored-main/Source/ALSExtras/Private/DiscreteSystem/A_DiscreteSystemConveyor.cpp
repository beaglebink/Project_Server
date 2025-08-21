#include "DiscreteSystem/A_DiscreteSystemConveyor.h"
#include "DiscreteSystem/A_DiscreteSystemNode.h"

AA_DiscreteSystemConveyor::AA_DiscreteSystemConveyor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));

	RootComponent = SceneComponent;
}

void AA_DiscreteSystemConveyor::BeginPlay()
{
	Super::BeginPlay();


}

void AA_DiscreteSystemConveyor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ArraysInitialization();
}

void AA_DiscreteSystemConveyor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_DiscreteSystemConveyor::OnNodeLogicFinished()
{
	++CurrentNodeIndex;
	if (CurrentNodeIndex == NodeActors.Num())
	{
		CurrentNodeIndex = 0;
	}
	else
	{
		StartSystemLogic();
	}
}

void AA_DiscreteSystemConveyor::OrderCorrection()
{
	for (AA_DiscreteSystemNode* SystemNode : NodeActors)
	{
		int32 Index = SystemNode->GetNodeNumber() - 1;
		if (Index >= NodeActors.Num())
		{
			continue;
		}
		NodeOrder[Index] = SystemNode->GetNodeNumberDefault() - 1;
	}
}

void AA_DiscreteSystemConveyor::ArraysInitialization()
{
	NodeActors.Empty();
	NodeOrder.Empty();

	int32 Index = 0;
	TArray<UChildActorComponent*> ChildComps;
	GetComponents<UChildActorComponent>(ChildComps);

	for (UChildActorComponent* ChildComp : ChildComps)
	{
		if (AA_DiscreteSystemNode* SystemNode = Cast<AA_DiscreteSystemNode>(ChildComp->GetChildActor()))
		{
			SystemNode->OnLogicFinished.Clear();
			SystemNode->OnLogicFinished.AddDynamic(this, &AA_DiscreteSystemConveyor::OnNodeLogicFinished);
			SystemNode->OnNumberChangedDel.AddDynamic(this, &AA_DiscreteSystemConveyor::OrderCorrection);
			NodeActors.Add(SystemNode);
			NodeOrder.Add(Index++);
		}
	}
}

void AA_DiscreteSystemConveyor::StartSystemLogic()
{
	int32 Index = NodeOrder[CurrentNodeIndex];
	if (Index >= NodeActors.Num())
	{
		OnNodeLogicFinished();
	}
	else
	{
		NodeActors[Index]->SetNodeActivation(true);
	}
}

void AA_DiscreteSystemConveyor::StopSystemLogic()
{
	CurrentNodeIndex = NodeActors.Num() - 1;
}

void AA_DiscreteSystemConveyor::ShuffleSystemLogic()
{
	for (int32 i = NodeOrder.Num() - 1; i > 0; --i)
	{
		int32 Index = FMath::RandRange(0, i);
		if (i != Index)
		{
			NodeOrder.Swap(i, Index);
		}
	}

	for (size_t i = 0; i < NodeActors.Num(); ++i)
	{
		NodeActors[NodeOrder[i]]->SetNodeNumber(FText::AsNumber(i + 1));
	}
}

