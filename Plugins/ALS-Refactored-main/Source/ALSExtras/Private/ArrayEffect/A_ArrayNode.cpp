#include "ArrayEffect/A_ArrayNode.h"
#include "AlsCharacterExample.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractivePickerComponent.h"
#include "Components/AudioComponent.h"
#include "ArrayEffect/A_ArrayEffect.h"

AA_ArrayNode::AA_ArrayNode()
{
	PrimaryActorTick.bCanEverTick = true;

	NodeBorder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NodeBorderComponent"));
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	MoveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("MoveTimeline"));
	NodeBorderAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("NodeAudioComponent"));

	RootComponent = SceneComponent;
	NodeBorder->SetupAttachment(RootComponent);
	NodeBorderAudioComp->SetupAttachment(RootComponent);
	NodeBorderAudioComp->bAutoActivate = false;
}

void AA_ArrayNode::BeginPlay()
{
	Super::BeginPlay();

	CurrentLocation = GetActorLocation();

	NodeBorder->OnComponentBeginOverlap.AddDynamic(this, &AA_ArrayNode::OnBeginOverlap);

	if (FloatCurve)
	{
		ProgressFunction.BindUFunction(this, FName("TimelineProgress"));
		MoveTimeline->AddInterpFloat(FloatCurve, ProgressFunction);

		FinishedFunction.BindUFunction(this, FName("TimelineFinished"));
		MoveTimeline->SetTimelineFinishedFunc(FinishedFunction);

		MoveTimeline->SetLooping(false);
	}
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
	if (!OtherActor || bIsOccupied || bIsMoving || !OwnerActor || OwnerActor->bIsOverlapping || OwnerActor->bIsSwapping || (NodeIndex == -1 && OwnerActor->NodeArray.Num() == 10)) return;

	OwnerActor->bIsOverlapping = true;

	ON_SCOPE_EXIT
	{
		OwnerActor->bIsOverlapping = false;
	};

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
			OtherComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECR_Ignore);
			OtherComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECR_Ignore);
			OtherComp->AttachToComponent(SceneComponent, FAttachmentTransformRules::KeepWorldTransform);
			GrabbedComponent = OtherComp;
			bIsOccupied = true;
			bShouldGrab = true;
			SetBorderMaterialIfOccupied(GrabbedComponent);

			if (NodeIndex == -1)
			{
				OnGrabDel.Broadcast();
			}
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

void AA_ArrayNode::DeleteNode()
{
	if (GrabbedComponent)
	{
		GrabbedComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECR_Block);
		GrabbedComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECR_Overlap);
		GrabbedComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		GrabbedComponent->SetSimulatePhysics(true);
		GrabbedComponent->AddImpulse((UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)->GetActorLocation() - GetActorLocation()).GetSafeNormal() * 1300.0f, NAME_None, true);
	}

	Destroy();
}

int32 AA_ArrayNode::GetIndex() const
{
	return NodeIndex;
}

void AA_ArrayNode::SetIndex(int32 NewIndex)
{
	if (NodeIndex != NewIndex)
	{
		NodeIndex = NewIndex;
		SetBorderMaterialAndIndex(NodeIndex);
	}
}

void AA_ArrayNode::SetBorderMaterialAndIndex(int32 NewIndex)
{
	if (!DMI_BorderMaterial)
	{
		if (NodeBorder && NodeBorder->GetMaterial(0))
		{
			DMI_BorderMaterial = NodeBorder->CreateDynamicMaterialInstance(0, NodeBorder->GetMaterial(0));
		}
	}

	if (DMI_BorderMaterial)
	{
		NodeBorder->SetMaterial(0, DMI_BorderMaterial);
		DMI_BorderMaterial->SetScalarParameterValue(FName("Index"), NewIndex);
	}
}

void AA_ArrayNode::SetBorderMaterialIfOccupied(UPrimitiveComponent* Occupant)
{
	DMI_BorderMaterial->SetScalarParameterValue(FName("Emissive"), 1.0f);

	if (Occupant)
	{
		DMI_BorderMaterial->SetScalarParameterValue(FName("Emissive"), 10.0f);
	}
}

void AA_ArrayNode::GetTextCommand(FText Command)
{
	OwnerActor->GetTextCommand(Command);
}

void AA_ArrayNode::AttachComponent(UPrimitiveComponent* OtherComp)
{
	OtherComp->AttachToComponent(SceneComponent, FAttachmentTransformRules::KeepWorldTransform);
}

void AA_ArrayNode::DetachComponent(UPrimitiveComponent* OtherComp)
{
	OtherComp->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
}

void AA_ArrayNode::MoveNode(FVector NewTargetLocation)
{
	bIsMoving = true;
	CurrentLocation = GetActorLocation();
	TargetLocation = NewTargetLocation;

	NodeBorderAudioComp->Play();

	MoveTimeline->PlayFromStart();
}

void AA_ArrayNode::TimelineProgress(float Value)
{
	SetActorLocation(FMath::Lerp(CurrentLocation, TargetLocation, Value));
}

void AA_ArrayNode::TimelineFinished()
{
	bIsMoving = false;
	NodeBorderAudioComp->Stop();
}