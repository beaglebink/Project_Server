#include "Pawns/P_Bubble.h"
#include "Components/SphereComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "AlsCharacterExample.h"
#include "AlsCameraComponent.h"
#include "AlsCharacterMovementComponent.h"

AP_Bubble::AP_Bubble()
{
	PrimaryActorTick.bCanEverTick = true;

	SphereCollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollisionComponent"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	FloatingPawnMovementComp = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingPawnMovementComponent"));

	RootComponent = SphereCollisionComponent;
	StaticMeshComponent->SetupAttachment(RootComponent);

	SphereCollisionComponent->SetSphereRadius(200.0f);
	FloatingPawnMovementComp->MaxSpeed = 500.0f;

	SphereCollisionComponent->SetCollisionProfileName("OverlapAllDynamic");

	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	StaticMeshComponent->SetCollisionProfileName("OverlapOnlyPawn");
	StaticMeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_PhysicsBody, ECollisionResponse::ECR_Overlap);

}

void AP_Bubble::BeginPlay()
{
	Super::BeginPlay();

	SphereCollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AP_Bubble::OnBeginOverlap);
	SphereCollisionComponent->OnComponentEndOverlap.AddDynamic(this, &AP_Bubble::OnEndOverlap);
}

void AP_Bubble::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!CatchedCharacter || !CatchedCharacter->bIsBubbled)
	{
		return;
	}

	DrawDebugSphere(GetWorld(), CatchedCharacter->GetActorLocation(), 20.0f, 8, FColor::Green, false, -1.0f, 0u, 3.0f);
	DrawDebugSphere(GetWorld(), GetActorLocation(), 20.0f, 8, FColor::Red, false, -1.0f, 0u, 3.0f);

	float DistanceToBubble = FVector::Distance(CatchedCharacter->GetActorLocation(), GetActorLocation());

	if (DistanceToBubble > 30.0f)
	{
		CatchedCharacter->SetActorLocation(FMath::VInterpTo(CatchedCharacter->GetActorLocation(), GetActorLocation(), GetWorld()->GetDeltaSeconds(), 6.0f));
	}
	else
	{
		FloatingPawnMovementComp->AddInputVector(FVector(CatchedCharacter->WindDirectionAndSpeed.X, CatchedCharacter->WindDirectionAndSpeed.Y, FloatingPawnMovementComp->GetMaxSpeed()).GetSafeNormal());
		CatchedCharacter->SetActorLocation(FMath::VInterpTo(CatchedCharacter->GetActorLocation(), GetActorLocation(), GetWorld()->GetDeltaSeconds(), 20.0f));
	}
}

void AP_Bubble::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	CatchedCharacter = Cast<AAlsCharacterExample>(OtherActor);

	if (!CatchedCharacter)
	{
		return;
	}

	CatchedCharacter->bIsBubbled = true;

	float CharacterGravity = CatchedCharacter->AlsCharacterMovement->GravityScale;
	CatchedCharacter->AlsCharacterMovement->Velocity = FVector::ZeroVector;
	CatchedCharacter->AlsCharacterMovement->SetMovementMode(EMovementMode::MOVE_Flying);

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [&, CharacterGravity]()
		{
			CatchedCharacter->bIsBubbled = false;
			CatchedCharacter->AlsCharacterMovement->GravityScale = CharacterGravity;
			CatchedCharacter->AlsCharacterMovement->SetMovementMode(EMovementMode::MOVE_Walking);

			Destroy();
		}, Time, false);
}

void AP_Bubble::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
}
