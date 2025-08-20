#include "DiscreteSystem/A_DiscreteSystemConveyor.h"
#include "DiscreteSystem/A_DiscreteSystemNode.h"

AA_DiscreteSystemConveyor::AA_DiscreteSystemConveyor()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AA_DiscreteSystemConveyor::BeginPlay()
{
	Super::BeginPlay();

	for (UChildActorComponent* ChildComp : NodeActors)
	{
		if (ChildComp && ChildComp->GetChildActor())
		{
			if (AA_DiscreteSystemNode* Node = Cast<AA_DiscreteSystemNode>(ChildComp->GetChildActor()))
			{
				Node->OnLogicFinished.AddDynamic(this, &AA_DiscreteSystemConveyor::OnNodeLogicFinished);
			}
		}
	}

	for (size_t i = 0; i < NodeActors.Num(); ++i)
	{
		NodeOrder[i] = i + 1;
	}
}

void AA_DiscreteSystemConveyor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_DiscreteSystemConveyor::OnNodeLogicFinished()
{
	if (CurrentNodeIndex == NodeActors.Num() - 1)
	{
		CurrentNodeIndex = 0;
	}
	else
	{
		if (AA_DiscreteSystemNode* Node = Cast<AA_DiscreteSystemNode>(NodeActors[NodeOrder[++CurrentNodeIndex]]))
		{
			Node->SetNodeActivation(true);
		}
	}
}

void AA_DiscreteSystemConveyor::StartSystemLogic()
{
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
}

