#include "Pawns/P_Bubble.h"
#include "Components/SphereComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "AlsCharacterExample.h"
#include "AlsCameraComponent.h"
#include "AlsCharacterMovementComponent.h"
#include "Components/TimelineComponent.h"

AP_Bubble::AP_Bubble()
{
	PrimaryActorTick.bCanEverTick = true;

	SphereCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollisionComponent"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	FloatingPawnMovementComp = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingPawnMovementComponent"));
	CatchTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("CatchTimeline"));

	RootComponent = StaticMeshComponent;
	SphereCollisionComponent->SetupAttachment(RootComponent);

	SphereCollisionComponent->SetSphereRadius(200.0f);
	FloatingPawnMovementComp->MaxSpeed = 500.0f;

	SphereCollisionComponent->SetCollisionProfileName(FName(TEXT("OverlapOnlyPawn")));

	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	StaticMeshComponent->SetCollisionProfileName(FName(TEXT("OverlapOnlyPawn")));
	StaticMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Overlap);
}

void AP_Bubble::BeginPlay()
{
	Super::BeginPlay();

	DistanceMeshToCollision = (SphereCollisionComponent->GetScaledSphereRadius() - StaticMeshComponent->Bounds.BoxExtent.X) / 10.0f;

	SphereCollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AP_Bubble::OnBeginOverlap);
	SphereCollisionComponent->OnComponentEndOverlap.AddDynamic(this, &AP_Bubble::OnEndOverlap);

	DynMaterial = UMaterialInstanceDynamic::Create(StaticMeshComponent->GetMaterial(0), this);
	StaticMeshComponent->SetMaterial(0, DynMaterial);

	if (FloatCurve)
	{
		ProgressFunction.BindUFunction(this, FName("TimelineProgress"));
		CatchTimeline->AddInterpFloat(FloatCurve, ProgressFunction);

		FinishedFunction.BindUFunction(this, FName("TimelineFinished"));
		CatchTimeline->SetTimelineFinishedFunc(FinishedFunction);

		CatchTimeline->SetLooping(false);
	}
}

void AP_Bubble::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!CaughtCharacter || !bIsCaught)
	{
		return;
	}

	float DistanceToBubble = FVector::Distance(CaughtCharacter->GetActorLocation(), GetActorLocation());

	if (DistanceToBubble > 30.0f)
	{
		CaughtCharacter->SetActorLocation(FMath::VInterpTo(CaughtCharacter->GetActorLocation(), GetActorLocation(), GetWorld()->GetDeltaSeconds(), 4.0f));
	}
	else
	{
		FloatingPawnMovementComp->AddInputVector(FVector(CaughtCharacter->WindDirectionAndSpeed.X, CaughtCharacter->WindDirectionAndSpeed.Y, FloatingPawnMovementComp->GetMaxSpeed()).GetSafeNormal());
		CaughtCharacter->SetActorLocation(FMath::VInterpTo(CaughtCharacter->GetActorLocation(), GetActorLocation(), GetWorld()->GetDeltaSeconds(), 20.0f));
	}
}

void AP_Bubble::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	CaughtCharacter = Cast<AAlsCharacterExample>(OtherActor);

	if (!CaughtCharacter)
	{
		return;
	}

	CaughtCharacter->bIsBubbled = true;

	CharacterGravity = CaughtCharacter->AlsCharacterMovement->GravityScale;
	PrevViewTag = CaughtCharacter->GetViewMode();

	CaughtCharacter->SetViewMode(CaughtCharacter->GetViewMode() == AlsViewModeTags::FirstPerson ? AlsViewModeTags::ThirdPerson : AlsViewModeTags::ThirdPerson);

	CaughtCharacter->AlsCharacterMovement->Velocity = FVector::ZeroVector;
	CaughtCharacter->GetMesh()->GetAnimInstance()->StopAllMontages(0.1f);
	CaughtCharacter->AlsCharacterMovement->SetMovementMode(EMovementMode::MOVE_Flying);

	DynMaterial->SetVectorParameterValue(TEXT("CharacterPosition"), CaughtCharacter->GetActorLocation());

	CatchTimeline->PlayFromStart();
}

void AP_Bubble::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}

void AP_Bubble::TimelineProgress(float Value)
{
	float Distance = FMath::Lerp(0.0f, DistanceMeshToCollision, Value);
	DynMaterial->SetScalarParameterValue(TEXT("CatchDistance"), Distance);
	CaughtCharacter->SetActorLocation(FMath::VInterpTo(CaughtCharacter->GetActorLocation(), GetActorLocation(), GetWorld()->GetDeltaSeconds(), 1.0f));
}

void AP_Bubble::TimelineFinished()
{
	bIsCaught = true;

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [&]()
		{
			CaughtCharacter->bIsBubbled = false;
			CaughtCharacter->AlsCharacterMovement->GravityScale = CharacterGravity;
			CaughtCharacter->AlsCharacterMovement->SetMovementMode(EMovementMode::MOVE_Walking);
			CaughtCharacter->SetViewMode(PrevViewTag);

			Destroy();
		}, Time, false);
}
