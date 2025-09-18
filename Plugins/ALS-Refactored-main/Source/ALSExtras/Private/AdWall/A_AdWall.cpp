#include "AdWall/A_AdWall.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AlsCharacterExample.h"
#include "Components/AudioComponent.h"

AA_AdWall::AA_AdWall()
{
	PrimaryActorTick.bCanEverTick = true;

	AdWallComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AdWallComponent"));
	CrossComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrossComponent"));
	MovementComp = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
	SpawnTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SpawnTimeline"));
	DestroyTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DestroyTimeline"));
	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));

	RootComponent = AdWallComp;
	CrossComp->SetupAttachment(RootComponent);
	CrossComp->SetRelativeLocation(FVector(2.02f, -140.0f, 90.0f));
	MovementComp->UpdatedComponent = RootComponent;
	MovementComp->InitialSpeed = 0.0f;
	MovementComp->ProjectileGravityScale = 0.0f;
	MovementComp->Velocity = FVector::ZeroVector;
	AudioComp->SetupAttachment(RootComponent);
	AudioComp->bAutoActivate = false;
}

#if WITH_EDITOR
void AA_AdWall::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		FName PropertyName = PropertyChangedEvent.Property->GetFName();

		if (PropertyName == GET_MEMBER_NAME_CHECKED(AA_AdWall, MinSpeed) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(AA_AdWall, MaxSpeed))
		{
			MaxSpeed = FMath::Max(MaxSpeed, MinSpeed);
		}

		if (PropertyName == GET_MEMBER_NAME_CHECKED(AA_AdWall, MinTime) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(AA_AdWall, MaxTime))
		{
			MaxTime = FMath::Max(MaxTime, MinTime);
		}
	}
}
#endif

void AA_AdWall::BeginPlay()
{
	Super::BeginPlay();

	AdWallComp->OnComponentHit.AddDynamic(this, &AA_AdWall::OnAdWallHit);

	AdWallComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AdWallComp->SetCollisionObjectType(ECC_PhysicsBody);
	AdWallComp->SetCollisionResponseToAllChannels(ECR_Block);

	if (SpawnFloatCurve)
	{
		SpawnProgressFunction.BindUFunction(this, FName("SpawnTimelineProgress"));
		SpawnTimeline->AddInterpFloat(SpawnFloatCurve, SpawnProgressFunction);

		SpawnFinishedFunction.BindUFunction(this, FName("SpawnTimelineFinished"));
		SpawnTimeline->SetTimelineFinishedFunc(SpawnFinishedFunction);

		SpawnTimeline->SetLooping(false);
	}
	if (DestroyFloatCurve)
	{
		DestroyProgressFunction.BindUFunction(this, FName("DestroyTimelineProgress"));
		DestroyTimeline->AddInterpFloat(DestroyFloatCurve, DestroyProgressFunction);

		DestroyFinishedFunction.BindUFunction(this, FName("DestroyTimelineFinished"));
		DestroyTimeline->SetTimelineFinishedFunc(DestroyFinishedFunction);

		DestroyTimeline->SetLooping(false);
	}

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
		AdWallComp->SetMassOverrideInKg(NAME_None, 100.0f);
		AdWallComp->SetLinearDamping(0.0f);
		AdWallComp->SetAngularDamping(0.0f);
		AdWallComp->SetConstraintMode(EDOFMode::XYPlane);
		ScheduleNextTimer();

		break;
	}
	case EnumAdType::Inflator:
	{
		break;
	}
	case EnumAdType::Malicious:
	{
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

void AA_AdWall::CallSpawnAd(bool bIsBumped)
{
	if (AdWallsMoreThan_25())
	{
		return;
	}

	float SpawnChance = FMath::FRandRange(0.0f, 100.0f);
	if (bIsBumped)
	{
		switch (AdType)
		{
		case EnumAdType::Standard:
		{
		}
		case EnumAdType::Drifter:
		{
			if (SpawnChance <= 20.0f)
			{
				SpawnAd();
			}
			break;
		}
		case EnumAdType::Inflator:
		{
			if (SpawnChance <= 30.0f)
			{
				SpawnAd();
			}
			break;
		}
		case EnumAdType::Malicious:
		{
			if (SpawnChance <= 25.0f)
			{
				SpawnAd();
			}
			break;
		}
		default:
		{
			break;
		}
		}
	}
	else
	{
		switch (AdType)
		{
		case EnumAdType::Standard:
		{
		}
		case EnumAdType::Drifter:
		{
			if (SpawnChance <= 20.0f)
			{
				SpawnAd();
			}
			break;
		}
		case EnumAdType::Inflator:
		{
			if (SpawnChance <= 30.0f)
			{
				SpawnAd();
			}
			break;
		}
		case EnumAdType::Malicious:
		{
			if (SpawnChance <= 25.0f)
			{
				SpawnAd();
			}
			break;
		}
		default:
		{
			break;
		}
		}
	}
}

void AA_AdWall::SpawnAd()
{
	if (!AdWallClass)
	{
		return;
	}

	FTransform SpawnTransform;
	SpawnTransform.SetLocation(GetActorLocation() + GetActorForwardVector() * 20.0f - GetActorRightVector() * 20.0f + GetActorUpVector() * 20.0f);
	SpawnTransform.SetRotation(GetActorRotation().Quaternion());
	SpawnTransform.SetScale3D(FVector(1.0f));

	if (AA_AdWall* Wall = GetWorld()->SpawnActorDeferred<AA_AdWall>(AdWallClass, SpawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
	{
		Wall->SpawnSound = SpawnSound;
		Wall->DestroySound = DestroySound;
		Wall->AdType = AdType;
		Wall->MinSpeed = MinSpeed;
		Wall->MaxSpeed = MaxSpeed;
		Wall->MinTime = MinTime;
		Wall->MaxTime = MaxTime;
		Wall->bShouldDoKnockback = bShouldDoKnockback;
		Wall->AdTexture = AdTexture;
		Wall->UpdateScreenMaterial();

		UGameplayStatics::FinishSpawningActor(Wall, SpawnTransform);

		Wall->SpawnTimeline->PlayFromStart();
		Wall->AudioComp->SetSound(SpawnSound);
		Wall->AudioComp->Play();
	}
}

void AA_AdWall::DriftAd()
{
	if (bIsHitAdWall || AdType != EnumAdType::Drifter)
	{
		return;
	}

	FVector ActorVelocity = FMath::VInterpTo(AdWallComp->GetPhysicsLinearVelocity(), TargetVelocity, GetWorld()->GetDeltaSeconds(), 1.0f);
	AdWallComp->SetPhysicsLinearVelocity(TargetVelocity);

	FRotator DeltaRot = AdWallComp->GetPhysicsLinearVelocity().GetSafeNormal().Rotation() - AdWallComp->GetComponentRotation();
	DeltaRot.Yaw = FMath::UnwindDegrees(DeltaRot.Yaw);

	AdWallComp->SetPhysicsAngularVelocityInDegrees(DeltaRot.Euler());
}

void AA_AdWall::DestroyAd()
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, "DESTROY");

	AudioComp->SetSound(DestroySound);
	AudioComp->Play();
	DestroyTimeline->PlayFromStart();
}

void AA_AdWall::ScheduleNextTimer()
{
	TargetVelocity = SetTargetVelocity();

	float Delay = FMath::FRandRange(MinTime, MaxTime);

	FTimerHandle RandomTimerHandle;
	GetWorldTimerManager().SetTimer(RandomTimerHandle, this, &AA_AdWall::ScheduleNextTimer, Delay, false);
}

FVector AA_AdWall::SetTargetVelocity()
{
	return FVector(FMath::FRandRange(-1.0f, 1.0f), FMath::FRandRange(-1.0f, 1.0f), 0.0f).GetSafeNormal() * FMath::FRandRange(MinSpeed, MaxSpeed);
}

void AA_AdWall::OnAdWallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bIsHitAdWall)
	{
		return;
	}

	bIsHitAdWall = true;

	AAlsCharacterExample* Ch = Cast<AAlsCharacterExample>(OtherActor);

	if (bShouldDoKnockback)
	{
		if (Ch)
		{
			FVector Direction = (Ch->GetActorLocation() - Hit.ImpactPoint).GetSafeNormal() * 3000.0f;
			Direction.Z = FMath::Clamp(Direction.Z, 0.0f, 1000.0f);
			Ch->LaunchCharacter(Direction, false, false);

			CallSpawnAd(true);
		}
	}

	FTimerHandle TimerHandleHit;
	GetWorldTimerManager().SetTimer(TimerHandleHit, [&]()
		{
			bIsHitAdWall = false;
		}, 0.3f, false);

	if (AdType == EnumAdType::Drifter && !Ch)
	{
		FVector Direction = -AdWallComp->GetPhysicsLinearVelocity().GetSafeNormal();
		float ImpulseStrength = FMath::GetMappedRangeValueClamped(FVector2D(MinSpeed, MaxSpeed), FVector2D(800.0f, 1000.0f), AdWallComp->GetPhysicsLinearVelocity().Length());

		AdWallComp->AddImpulseAtLocation(Direction * ImpulseStrength, Hit.ImpactPoint);

		FTimerHandle TimerHandleVelocity;
		GetWorldTimerManager().SetTimer(TimerHandleVelocity, [&]()
			{
				TargetVelocity = SetTargetVelocity();
			}, 0.29f, false);
	}

	if (AdType == EnumAdType::Malicious && Ch)
	{
		UGameplayStatics::ApplyDamage(Ch, FMath::FRandRange(FMath::Max(0.0f, AdDamage - 5.0f), AdDamage + 5.0f), Ch->GetController(), this, UDamageType::StaticClass());
	}
}

void AA_AdWall::UpdateScreenMaterial()
{
	if (!AdTexture)
	{
		return;
	}


	if (!DynamicMaterial && AdWallComp->GetMaterial(0))
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(AdWallComp->GetMaterial(0), this);
		AdWallComp->SetMaterial(0, DynamicMaterial);
	}

	if (DynamicMaterial)
	{
		DynamicMaterial->SetTextureParameterValue(FName("Screen"), AdTexture);
	}
}

void AA_AdWall::HandleWeaponShot_Implementation(const FHitResult& Hit)
{
	if (Hit.Component == AdWallComp)
	{
		CallSpawnAd(false);
	}
	else if (Hit.Component == CrossComp)
	{
		DestroyAd();
	}
}

void AA_AdWall::SpawnTimelineProgress(float Value)
{
	float ScaleValue = FMath::Lerp(0.01f, 1.0f, Value);
	AdWallComp->SetWorldScale3D(FVector(ScaleValue));
}

void AA_AdWall::SpawnTimelineFinished()
{
	CrossComp->RecreatePhysicsState();
	AudioComp->Stop();
}

void AA_AdWall::DestroyTimelineProgress(float Value)
{
}

void AA_AdWall::DestroyTimelineFinished()
{
	AudioComp->Stop();
	Destroy();
}
