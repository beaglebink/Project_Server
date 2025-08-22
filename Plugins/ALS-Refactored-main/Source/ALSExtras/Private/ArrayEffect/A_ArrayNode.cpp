#include "ArrayEffect/A_ArrayNode.h"

AA_ArrayNode::AA_ArrayNode()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	NodeBorder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NodeBorderComponent"));
	NodeContainer = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NodeContainerComponent"));

	RootComponent = SceneComponent;
	NodeBorder->SetupAttachment(RootComponent);
	NodeContainer->SetupAttachment(RootComponent);
}

void AA_ArrayNode::BeginPlay()
{
	Super::BeginPlay();

	NodeBorder->OnComponentBeginOverlap.AddDynamic(this, &AA_ArrayNode::OnBeginOverlap);
}

void AA_ArrayNode::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SetBorderMaterialAndIndex();
}

void AA_ArrayNode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_ArrayNode::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, OtherActor->GetName());
}

void AA_ArrayNode::SetBorderMaterialAndIndex(int32 NewIndex)
{
	if (!DMI_BorderMaterial)
	{
		if (NodeBorder && NodeBorder->GetMaterial(0))
		{
			DMI_BorderMaterial = NodeBorder->CreateDynamicMaterialInstance(0, NodeBorder->GetMaterial(0));
			NodeBorder->SetMaterial(0, DMI_BorderMaterial);
		}
	}

	if (DMI_BorderMaterial)
	{
		DMI_BorderMaterial->SetScalarParameterValue(FName("Index"), NewIndex);
	}
}
