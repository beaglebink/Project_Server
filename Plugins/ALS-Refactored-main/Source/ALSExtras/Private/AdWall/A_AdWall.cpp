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

	AdWallComp->OnComponentBeginOverlap.AddDynamic(this, &AA_AdWall::OnAdWallBeginOverlap);
	AdWallComp->OnComponentHit.AddDynamic(this, &AA_AdWall::OnAdWallHit);
	CrossComp->OnComponentHit.AddDynamic(this, &AA_AdWall::OnCrossHit);

	switch (AdType)
	{
	case EnumAdType::Standard:
	{
		break;
	}
	case EnumAdType::Drifter:
	{
		AdWallComp->SetSimulatePhysics(true);
		AdWallComp->SetNotifyRigidBodyCollision(true);
		AdWallComp->SetEnableGravity(false);
		AdWallComp->SetMassOverrideInKg(NAME_None, 1.0f);
		AdWallComp->SetLinearDamping(0.0f);
		AdWallComp->SetAngularDamping(0.0f);
		AdWallComp->SetConstraintMode(EDOFMode::XYPlane);

		AdWallComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AdWallComp->SetCollisionObjectType(ECC_PhysicsBody);

		AdWallComp->SetCollisionResponseToAllChannels(ECR_Block);
		AdWallComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
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
	if (bIsHitAdWall)
	{
		return;
	}

	FVector ActorVelocity = FMath::VInterpTo(AdWallComp->GetPhysicsLinearVelocity(), TargetVelocity, GetWorld()->GetDeltaSeconds(), 1.0f);
	AdWallComp->SetPhysicsLinearVelocity(TargetVelocity);

	FRotator DeltaRot = AdWallComp->GetPhysicsLinearVelocity().GetSafeNormal().Rotation() - AdWallComp->GetComponentRotation();
	DeltaRot.Yaw = FMath::UnwindDegrees(DeltaRot.Yaw);
	
	AdWallComp->SetPhysicsAngularVelocityInDegrees(DeltaRot.Euler());
}

void AA_AdWall::ScheduleNextTimer()
{
	TargetVelocity = SetTargetVelocity(70.0f, 120.0f);

	float Delay = FMath::FRandRange(5.0f, 8.0f);

	FTimerHandle RandomTimerHandle;
	GetWorldTimerManager().SetTimer(RandomTimerHandle, this, &AA_AdWall::ScheduleNextTimer, Delay, false);
}

FVector AA_AdWall::SetTargetVelocity(float MinVelocity, float MaxVelocity)
{
	return FVector(FMath::FRandRange(-1.0f, 1.0f), FMath::FRandRange(-1.0f, 1.0f), 0.0f).GetSafeNormal() * FMath::FRandRange(MinVelocity, MaxVelocity);
}

void AA_AdWall::OnAdWallBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, OtherActor->GetName());
}

void AA_AdWall::OnAdWallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Blue, OtherActor->GetName());
	if (bIsHitAdWall)
	{
		return;
	}

	bIsHitAdWall = true;

	FTimerHandle TimerHandleHit;
	GetWorldTimerManager().SetTimer(TimerHandleHit, [&]()
		{
			bIsHitAdWall = false;
		}, 0.3f, false);
	FVector Direction = -AdWallComp->GetPhysicsLinearVelocity().GetSafeNormal();
	float ImpulseStrength = FMath::GetMappedRangeValueClamped(FVector2D(70.0f, 120.0f), FVector2D(400.0f, 500.0f), AdWallComp->GetPhysicsLinearVelocity().Length());

	AdWallComp->AddImpulseAtLocation(Direction * ImpulseStrength, Hit.ImpactPoint);

	FTimerHandle TimerHandleVelocity;
	GetWorldTimerManager().SetTimer(TimerHandleVelocity, [&]()
		{
			TargetVelocity = SetTargetVelocity(70.0f, 120.0f);
		}, 0.29f, false);
}

void AA_AdWall::OnCrossHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, OtherActor->GetName());
}
