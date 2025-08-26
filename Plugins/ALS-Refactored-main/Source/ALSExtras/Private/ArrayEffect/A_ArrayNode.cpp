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
	if (!OtherActor || bIsOccupied || bIsMoving) return;

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

			OnGrabDel.Broadcast();
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
	GrabbedComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECR_Block);
	GrabbedComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECR_Overlap);
	GrabbedComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	GrabbedComponent->SetSimulatePhysics(true);
	GrabbedComponent->AddImpulse((UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)->GetActorLocation() - GetActorLocation()).GetSafeNormal() * 1300.0f, NAME_None, true);

	OnDeleteDel.Broadcast(NodeIndex);

	Destroy();
}

bool AA_ArrayNode::ParseArrayIndexToSwap(FText Command, int32& OutIndex)
{
	FString Input = Command.ToString();

	int32 OpenBracket = 0;
	int32 CloseBracket = 0;

	if (!Input.FindChar('[', OpenBracket) || !Input.FindChar(']', CloseBracket) || OpenBracket >= CloseBracket || Input.Left(OpenBracket) != "arr") return false;

	FString IndexStr = Input.Mid(OpenBracket + 1, CloseBracket - OpenBracket - 1);

	if (!IndexStr.IsNumeric()) return false;

	OutIndex = FCString::Atoi(*IndexStr);

	return true;
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

void AA_ArrayNode::GetTextCommand(FText Command)
{
	if (NodeIndex == -1)
	{
		return;
	}

	//delete
	if (Command.ToString() == "del")
	{
		DeleteNode();
	}

	//swap
	int32 OutIndex = -1;
	if (ParseArrayIndexToSwap(Command, OutIndex))
	{
		OwnerActor->SwapNode(NodeIndex, OutIndex);
	}
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