#include "DiscreteSystem/A_DiscreteSystemNode.h"

AA_DiscreteSystemNode::AA_DiscreteSystemNode()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SM_ZoneBorder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneBorder"));
	SKM_Node = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("NodeMesh"));

	RootComponent = SceneComponent;
	SM_ZoneBorder->SetupAttachment(RootComponent);
	SKM_Node->SetupAttachment(SM_ZoneBorder);
}

void AA_DiscreteSystemNode::BeginPlay()
{
	Super::BeginPlay();

}

void AA_DiscreteSystemNode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_DiscreteSystemNode::UpdateBorderMaterial()
{
	CurrentNodeNumber = NodeNumber;

	if (SM_ZoneBorder && SM_ZoneBorder->GetMaterial(0))
	{
		DMI_BorderMaterial = SM_ZoneBorder->CreateDynamicMaterialInstance(0, SM_ZoneBorder->GetMaterial(0));
		if (DMI_BorderMaterial)
		{
			SM_ZoneBorder->SetMaterial(0, DMI_BorderMaterial);
			DMI_BorderMaterial->SetScalarParameterValue(FName("NodeNumber"), CurrentNodeNumber);
		}
	}
}

