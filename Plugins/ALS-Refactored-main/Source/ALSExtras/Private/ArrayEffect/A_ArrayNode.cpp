#include "ArrayEffect/A_ArrayNode.h"
#include "AlsCharacterExample.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractivePickerComponent.h"

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

	ComponentGrabbing();
}

void AA_ArrayNode::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || bIsOccupied) return;

	if (AAlsCharacterExample* Player = Cast<AAlsCharacterExample>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		if (UPhysicsHandleComponent* PhysicsHandleComp = Player->GetComponentByClass<UPhysicsHandleComponent>())
		{
			if (PhysicsHandleComp->GrabbedComponent == OtherComp)
			{
				if (UInteractivePickerComponent* Picker = Player->GetComponentByClass<UInteractivePickerComponent>())
				{
					Picker->DoInteractiveUse();
				}
			}

			OtherComp->SetSimulatePhysics(false);
			OtherComp->AttachToComponent(SceneComponent, FAttachmentTransformRules::KeepWorldTransform);
			GrabbedComponent = OtherComp;
			bIsOccupied = true;
			bShouldGrab = true;
		}
	}
}

void AA_ArrayNode::ComponentGrabbing()
{
	if (bShouldGrab)
	{
		GrabbedComponent->SetWorldTransform(FTransform(FMath::RInterpTo(GrabbedComponent->GetComponentRotation(), GetActorRotation(), GetWorld()->GetDeltaSeconds(), 1.0f),
			FMath::VInterpTo(GrabbedComponent->GetComponentLocation(), GetActorLocation(), GetWorld()->GetDeltaSeconds(), 1.0f), FVector(1.0f, 1.0f, 1.0f)));

		if (GrabbedComponent->GetComponentLocation().Equals(GetActorLocation(), 0.01f) && GrabbedComponent->GetComponentRotation().Equals(GetActorRotation(), 0.01f))
		{
			bShouldGrab = false;
		}
	}
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
