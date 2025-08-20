#include "DiscreteSystem/A_DiscreteSystemNode.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

AA_DiscreteSystemNode::AA_DiscreteSystemNode()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SM_ZoneBorder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZoneBorder"));
	SKM_Node = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("NodeMesh"));
	NodeAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("NodeAudio"));

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

void AA_DiscreteSystemNode::SetNodeActivation(bool IsActive)
{
	if (bIsActivated == IsActive)
	{
		return;
	}

	bIsActivated = IsActive;
	OnActivationChanged();
}

bool AA_DiscreteSystemNode::GetNodeActivation() const
{
	return bIsActivated;
}

void AA_DiscreteSystemNode::SetNodeNumber(FText NewNumber)
{
	int NumberToSet = FCString::Atoi(*NewNumber.ToString());
	if (NumberToSet < 1 || NumberToSet > 9 || CurrentNodeNumber == NumberToSet)
	{
		return;
	}

	CurrentNodeNumber = NumberToSet;
	OnNumberChanged();
}

int32 AA_DiscreteSystemNode::GetNodeNumber() const
{
	return CurrentNodeNumber;
}

void AA_DiscreteSystemNode::OnActivationChanged()
{
	//Make border material lighter to show that node is active
	if (DMI_BorderMaterial)
	{
		DMI_BorderMaterial->SetScalarParameterValue(FName("Emissive"), 1.0f + 10.0f * bIsActivated);
	}

	//Node logic if active
	if (bIsActivated)
	{

	}
}

void AA_DiscreteSystemNode::OnNumberChanged()
{
	if (DMI_BorderMaterial)
	{
		DMI_BorderMaterial->SetScalarParameterValue(FName("InTheCorrectOrder"), NodeNumber - CurrentNodeNumber);
		DMI_BorderMaterial->SetScalarParameterValue(FName("NodeNumber"), CurrentNodeNumber);
	}

	NodeSound();
}

void AA_DiscreteSystemNode::NodeSound()
{
	if (NodeSoundCorrectWork && NodeSoundUncorrectWork)
	{
		NodeAudio->Stop();
		if (NodeNumber - CurrentNodeNumber)
		{
			NodeAudio = UGameplayStatics::SpawnSoundAtLocation(GetWorld(), NodeSoundUncorrectWork, GetActorLocation(), GetActorRotation(), 1.0f);
		}
		else
		{
			NodeAudio = UGameplayStatics::SpawnSoundAtLocation(GetWorld(), NodeSoundCorrectWork, GetActorLocation(), GetActorRotation(), 1.0f);
		}
	}
}

