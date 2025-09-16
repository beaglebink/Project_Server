#include "AdWall/A_AdWall.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AA_AdWall::AA_AdWall()
{
	PrimaryActorTick.bCanEverTick = true;

	AdWallComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AdWallComponent"));
	CrossComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrossComponent"));
	MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));

	RootComponent = AdWallComp;
	CrossComp->SetupAttachment(RootComponent);
	CrossComp->SetRelativeLocation(FVector(2.02f, -140.0f, 90.0f));
	MovementComp->UpdatedComponent = RootComponent;
}

void AA_AdWall::BeginPlay()
{
	Super::BeginPlay();

	switch (AdType)
	{
	case EnumAdType::Standard:
	{
		break;
	}
	case EnumAdType::Drifter:
	{
		AdWallComp->SetSimulatePhysics(true);
		AdWallComp->SetEnableGravity(false);
		AdWallComp->SetMassOverrideInKg(NAME_None, 1.0f);
		AdWallComp->SetLinearDamping(0.0f);
		AdWallComp->SetAngularDamping(0.0f);
		AdWallComp->SetConstraintMode(EDOFMode::XYPlane);

		AdWallComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AdWallComp->SetCollisionObjectType(ECC_PhysicsBody);

		AdWallComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		AdWallComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		AdWallComp->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
		ScheduleNextTimer();

		break;
	}
	case EnumAdType::Inflator:
	{
		break;
	}
	case EnumAdType::Malicious:
	{
		AdDamage = FMath::FRandRange(10.0f, 20.0f);
		break;
	}
	default:
	{
		break;
	}
	}
}

void AA_AdWall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DriftAd();
}

bool AA_AdWall::AdWallsMoreThan_25()
{
	TArray<AActor*> Ads;

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), this->StaticClass(), Ads);

	return Ads.Num() >= 25;
}

void AA_AdWall::SpawnAd()
{
}

void AA_AdWall::DriftAd()
{
	FVector ActorVelocity = FMath::VInterpTo(AdWallComp->GetPhysicsLinearVelocity(), TargetVelocity, GetWorld()->GetDeltaSeconds(), 1.0f);
	AdWallComp->SetPhysicsLinearVelocity(TargetVelocity);

	FVector DeltaRot = (AdWallComp->GetPhysicsLinearVelocity().GetSafeNormal().Rotation() - AdWallComp->GetComponentRotation()).Euler();
	AdWallComp->SetPhysicsAngularVelocityInDegrees(DeltaRot);
}

void AA_AdWall::ScheduleNextTimer()
{
	TargetVelocity = FVector(FMath::FRandRange(-1.0f, 1.0f), FMath::FRandRange(-1.0f, 1.0f), 0.0f).GetSafeNormal() * FMath::FRandRange(170.0f, 220.0f);

	float Delay = FMath::FRandRange(5.0f, 8.0f);

	FTimerHandle RandomTimerHandle;
	GetWorldTimerManager().SetTimer(RandomTimerHandle, this, &AA_AdWall::ScheduleNextTimer, Delay, false);
}