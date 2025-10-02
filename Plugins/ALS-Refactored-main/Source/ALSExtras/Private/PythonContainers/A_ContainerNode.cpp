#include "PythonContainers/A_ContainerNode.h"
#include "PythonContainers/A_PythonContainer.h"
#include "AlsCharacterExample.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractivePickerComponent.h"
#include "Components/AudioComponent.h"

AA_ContainerNode::AA_ContainerNode()
{
	PrimaryActorTick.bCanEverTick = true;

	NodeBorder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NodeBorderComponent"));
	MoveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("MoveTimeline"));
	NodeBorderAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("NodeAudioComponent"));

	RootComponent = NodeBorder;
	NodeBorderAudioComp->SetupAttachment(RootComponent);

	NodeBorder->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	NodeBorder->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	NodeBorder->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	NodeBorder->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel2, ECollisionResponse::ECR_Overlap);
	NodeBorder->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel3, ECollisionResponse::ECR_Overlap);
	NodeBorder->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel5, ECollisionResponse::ECR_Block);
	NodeBorder->SetGenerateOverlapEvents(true);

	NodeBorderAudioComp->bAutoActivate = false;
}

void AA_ContainerNode::BeginPlay()
{
	Super::BeginPlay();

	DefaultLocation = GetActorLocation();

	CurrentLocation = GetActorLocation();

	NodeBorder->OnComponentBeginOverlap.AddDynamic(this, &AA_ContainerNode::OnBeginOverlap);

	if (FloatCurve)
	{
		ProgressFunction.BindUFunction(this, FName("TimelineProgress"));
		MoveTimeline->AddInterpFloat(FloatCurve, ProgressFunction);

		FinishedFunction.BindUFunction(this, FName("TimelineFinished"));
		MoveTimeline->SetTimelineFinishedFunc(FinishedFunction);

		MoveTimeline->SetLooping(false);
	}
}

void AA_ContainerNode::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SetBorderMaterialAndIndex();
}

void AA_ContainerNode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ComponentGrabbing();
}

void AA_ContainerNode::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || bIsOccupied || bIsMoving || !OwnerActor || OwnerActor == OtherActor || OwnerActor->bIsOverlapping ||
		OwnerActor->bIsSwapping || OwnerActor->bIsOnConcatenation || OwnerActor->bIsDetaching || (NodeIndex == -1 && OwnerActor->NodeArray.Num() == 10)) return;

	OwnerActor->bIsOverlapping = true;

	ON_SCOPE_EXIT
	{
		OwnerActor->bIsOverlapping = false;
	};

	//handle concatenation
	if (AA_PythonContainer* PythonCont = Cast<AA_PythonContainer>(OtherActor))
	{
		if (PythonCont->bIsOnConcatenation)
		{
			OwnerActor->ContainerConcatenate(PythonCont);
		}
		return;
	}

	//grab mesh component
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

			TryGrabActor(OtherActor);
		}
	}
}

void AA_ContainerNode::TryGrabActor(AActor* OtherActor)
{
	if (!OtherActor)
	{
		return;
	}

	if (UStaticMeshComponent* MeshComp = OtherActor->FindComponentByClass<UStaticMeshComponent>())
	{
		if (!MeshComp) return;

		MeshComp->SetSimulatePhysics(false);
		MeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECR_Ignore);
		MeshComp->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECR_Ignore);
		OtherActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);

		GrabbedActor = OtherActor;
		GrabbedComponent = MeshComp;
		bIsOccupied = true;
		bShouldGrab = true;

		SetBorderMaterialIfOccupied(GrabbedComponent);

		if (NodeIndex == -1)
		{
			OwnerActor->AppendNode();
		}
	}
}

void AA_ContainerNode::ComponentGrabbing()
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

void AA_ContainerNode::DeleteNode()
{
	if (GrabbedComponent)
	{
		GrabbedComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECR_Block);
		GrabbedComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECR_Overlap);
		GrabbedActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		GrabbedComponent->SetSimulatePhysics(true);
		GrabbedComponent->AddImpulse((UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)->GetActorLocation() - GetActorLocation()).GetSafeNormal() * 1300.0f, NAME_None, true);
	}

	Destroy();
}

int32 AA_ContainerNode::GetIndex() const
{
	return NodeIndex;
}

void AA_ContainerNode::SetIndex(int32 NewIndex)
{
	if (NodeIndex != NewIndex)
	{
		NodeIndex = NewIndex;
		SetBorderMaterialAndIndex(NodeIndex);
	}
}

void AA_ContainerNode::SetBorderMaterialAndIndex(int32 NewIndex)
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

void AA_ContainerNode::SetBorderMaterialIfOccupied(UPrimitiveComponent* Occupant)
{
	DMI_BorderMaterial->SetScalarParameterValue(FName("Emissive"), 1.0f);

	if (Occupant)
	{
		DMI_BorderMaterial->SetScalarParameterValue(FName("Emissive"), 10.0f);
	}
}

void AA_ContainerNode::GetTextCommand(FText Command)
{
	OwnerActor->GetTextCommand(Command);
}

void AA_ContainerNode::AttachToNode(AActor* OtherActor)
{
	OtherActor->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
}

void AA_ContainerNode::DetachFromNode(AActor* OtherActor)
{
	OtherActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

void AA_ContainerNode::MoveNode(FVector NewTargetLocation)
{
	bIsMoving = true;
	CurrentLocation = GetActorLocation();
	TargetLocation = NewTargetLocation;

	NodeBorderAudioComp->Play();

	MoveTimeline->PlayFromStart();
}

void AA_ContainerNode::TimelineProgress(float Value)
{
	SetActorLocation(FMath::Lerp(CurrentLocation, TargetLocation, Value));
}

void AA_ContainerNode::TimelineFinished()
{
	bIsMoving = false;
	NodeBorderAudioComp->Stop();

	DefaultLocation = GetActorLocation();
}