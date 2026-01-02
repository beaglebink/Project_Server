#include "DebrisSystem/A_Debris.h"
#include "Engine/TextureRenderTarget2D.h" 
#include "Kismet/KismetRenderingLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"

AA_Debris::AA_Debris()
{
	PrimaryActorTick.bCanEverTick = true;

	DebrisMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebrisMeshComponent"));
	DebrisFlowFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DebrisFlowFXComponent"));

	RootComponent = DebrisMeshComponent;
	DebrisFlowFXComponent->SetupAttachment(DebrisMeshComponent);

	DebrisFlowFXComponent->SetAutoActivate(false);
}

void AA_Debris::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AA_Debris::BeginPlay()
{
	Super::BeginPlay();

	if (bShouldMeshSimulatePhysics)
	{
		DebrisMeshComponent->SetSimulatePhysics(true);
	}

	if (!IsValid(MeshMaterialInstanceDynamic))
	{
		MeshMaterialInstanceDynamic = DebrisMeshComponent->CreateAndSetMaterialInstanceDynamic(0);
	}

	MeshRenderTarget = NewObject<UTextureRenderTarget2D>(this);
	if (!MeshRenderTarget)return;

	MeshRenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
	MeshRenderTarget->InitAutoFormat(1024, 1024);
	MeshRenderTarget->ClearColor = FLinearColor::Black;
	MeshRenderTarget->UpdateResourceImmediate(true);

	UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), MeshRenderTarget, MeshMaterialInstanceDynamic);
	if (ParticlesMaterial)
	{
		UMaterialInstanceDynamic* FXDynMat = nullptr;
		DebrisFlowFXComponent->SetVariableObject(TEXT("User.SourceMesh"), DebrisMeshComponent->GetStaticMesh());
		FXDynMat = UMaterialInstanceDynamic::Create(ParticlesMaterial, this);
		FXDynMat->SetTextureParameterValue(TEXT("MeshRenderTarget"), MeshRenderTarget);
		DebrisFlowFXComponent->SetVariableMaterial(TEXT("User.RenderMaterial"), FXDynMat);
	}

	//Initialize floating behavior variables
	BaseLocation = GetActorLocation();
	TargetLocation = BaseLocation;
	InitialDirection = FMath::VRand();
	NoiseSeed = FMath::FRandRange(0.0f, 10.0f);
	OrbitRadius = FMath::FRandRange(20.0f, 100.0f);
	OrbitStartOffset = FMath::VRand() * OrbitRadius;

	//Initialize attachment direction
	UStaticMeshComponent* MeshComp = GetOwner()->FindComponentByClass<UStaticMeshComponent>();
	if (MeshComp)
	{
		AttachmentDirection = (DebrisMeshComponent->GetComponentLocation() - MeshComp->GetComponentLocation()).GetSafeNormal();
	}

}

void AA_Debris::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	BaseLocation = FMath::VInterpTo(BaseLocation, TargetLocation, DeltaTime, 5.0f);

	DebrisFloatBehavior(DeltaTime);

	DebrisReactToPlayer(DeltaTime);

	DebrisBuoyancyBehavior(DeltaTime);
}

void AA_Debris::DestroyOrRespawnDebris()
{
	if (ReplenishTime > 0.0f)
	{
		if (!GetWorldTimerManager().IsTimerActive(RespawnTimerHandle))
		{
			DebrisMeshComponent->SetVisibility(false);
			DebrisMeshComponent->SetSimulatePhysics(false);
			DebrisMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			SetActorTickEnabled(false);
			Alpha = 0.0f;

			GetWorldTimerManager().SetTimer(RespawnTimerHandle, [this]()
				{
					DebrisMeshComponent->SetVisibility(true);
					DebrisMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					if (bShouldMeshSimulatePhysics)
					{
						DebrisMeshComponent->SetSimulatePhysics(true);
					}
					DebrisMeshComponent->SetWorldScale3D(FVector(1.0f));
					SetActorTickEnabled(true);
				}, ReplenishTime, false);
		}
	}
	else
	{
		Destroy();
	}
}

float AA_Debris::DebrisMassInfluence() const
{
	FVector DebrisMeshBoxExtent = DebrisMeshComponent->Bounds.BoxExtent;
	return FMath::Clamp(DebrisMeshBoxExtent.X * DebrisMeshBoxExtent.Y * DebrisMeshBoxExtent.Z / 1000.0f, 1.0f, 1000.0f) * SuctionDifficultyMultiplier;
}

void AA_Debris::CheckIfDebrisDetached(FVector DeltaForDetachment, float Effectiveness)
{
	if (AttachmentPower == 0.0f) return;

	float DetachmentPower = DeltaForDetachment.Size() * FVector::DotProduct(AttachmentDirection, DeltaForDetachment.GetSafeNormal()) * Effectiveness;
	if (AttachmentPower <= DetachmentPower)
	{
		TargetLocation += DeltaForDetachment;
		AttachmentPower = 0.0f;
	}
}

void AA_Debris::DebrisFloatBehavior(float DeltaTime)
{
	TimeAccumulator += DeltaTime;
	switch (DebrisBehavior)
	{
	case EDebrisFloatPattern::Stationary:
	{
		float Offset = FMath::Sin(TimeAccumulator * 1.5f + NoiseSeed) * 2.0f;
		SetActorLocation(BaseLocation + FVector(0, 0, Offset));
		break;
	}
	case EDebrisFloatPattern::Drift:
	{
		SetActorLocation(GetActorLocation() + InitialDirection * DebrisMovementSpeed * DeltaTime);
		break;
	}
	case EDebrisFloatPattern::Wander:
	{
		FVector WanderOffset;
		WanderOffset.X = FMath::Sin(TimeAccumulator * 0.7f + NoiseSeed);
		WanderOffset.Y = FMath::Cos(TimeAccumulator * 0.9f + NoiseSeed * 2.0f);
		WanderOffset.Z = FMath::Sin(TimeAccumulator * 0.5f);

		SetActorLocation(BaseLocation + WanderOffset * DebrisMovementSpeed);
		break;
	}
	case EDebrisFloatPattern::Orbit:
	{
		const FQuat RotationQuat(InitialDirection, TimeAccumulator * DebrisMovementSpeed / 10.0f);

		const FVector RotatedOffset = RotationQuat.RotateVector(OrbitStartOffset);

		SetActorLocation(BaseLocation + RotatedOffset);
		break;
	}
	case EDebrisFloatPattern::Erratic:
	{
		FVector Noise;
		Noise.X = FMath::PerlinNoise1D(TimeAccumulator * 3.0f + NoiseSeed);
		Noise.Y = FMath::PerlinNoise1D(TimeAccumulator * 4.0f + NoiseSeed * 2);
		Noise.Z = FMath::PerlinNoise1D(TimeAccumulator * 5.0f + NoiseSeed * 3);

		FVector TargetOffset = Noise * DebrisMovementSpeed * ErraticRadius;

		ErraticOffset = FMath::VInterpTo(ErraticOffset, TargetOffset, DeltaTime, 5.0f);

		SetActorLocation(BaseLocation + ErraticOffset);
		break;
	}
	default:
		break;
	}
}

void AA_Debris::DebrisReactToPlayer(float DeltaTime)
{
	APawn* PlayerPawn = GetWorld()->GetFirstPlayerController()->GetPawn();
	if (!PlayerPawn) return;

	FVector ToPlayer = PlayerPawn->GetActorLocation() - GetActorLocation();
	float Distance = ToPlayer.Size();
	FVector DirToPlayer = ToPlayer.GetSafeNormal();
	float InfluenceAlpha = 1.0f - FMath::Clamp(Distance / PlayerInfluenceRadius, 0.0f, 1.0f);

	UCharacterMovementComponent* MoveComp = PlayerPawn->FindComponentByClass<UCharacterMovementComponent>();
	FVector PlayerVelocity = MoveComp ? MoveComp->Velocity : FVector::ZeroVector;

	switch (PlayerReaction)
	{
	case EDebrisReactToPlayer::Passive:
		break;
	case EDebrisReactToPlayer::Evasive:
	{
		Evasion = FMath::VInterpTo(Evasion, -DirToPlayer * PlayerInfluenceStrength * 100.0f * InfluenceAlpha, DeltaTime, 1.0f);
		SetActorLocation(GetActorLocation() + Evasion);
		break;
	}
	case EDebrisReactToPlayer::Attracted:
	{
		Attraction = FMath::VInterpTo(Attraction, DirToPlayer * PlayerInfluenceStrength * 100.0f * InfluenceAlpha, DeltaTime, 1.0f);
		SetActorLocation(GetActorLocation() + Attraction);
		break;
	}
	case EDebrisReactToPlayer::Neutral:
	{
		SpeedInfluenceAlpha = FMath::FInterpTo(SpeedInfluenceAlpha, FMath::Clamp(PlayerVelocity.Size() / MoveComp->GetMaxSpeed(), 0.0f, 1.0f), DeltaTime, 1.0f);
		CurrentVelocitySize = FMath::FInterpTo(CurrentVelocitySize, PlayerVelocity.Size(), DeltaTime, 1.0f);
		Turbulence = FMath::VInterpTo(Turbulence, DirToPlayer * FVector::DotProduct(PlayerVelocity.GetSafeNormal(), DirToPlayer) * PlayerInfluenceStrength
			* InfluenceAlpha * FMath::Pow(SpeedInfluenceAlpha, 2) * CurrentVelocitySize, DeltaTime, 1.0f);
		SetActorLocation(GetActorLocation() + Turbulence);
		break;
	}
	default:
		break;
	}
}

void AA_Debris::DebrisBuoyancyBehavior(float DeltaTime)
{
	switch (BuoyancyType)
	{
	case EBuoyancyType::Fixed:
		break;
	case EBuoyancyType::Floating:
	{
		BuoyancyDistanceAccumulator += DeltaTime * BuoyancySpeed * BuoyancyDirection;
		if (BuoyancyDistanceAccumulator >= BuoyancyRange)
		{
			BuoyancyDistanceAccumulator = BuoyancyRange;
			BuoyancyDirection = -1.0f;
		}
		if (BuoyancyDistanceAccumulator <= 0.0f)
		{
			BuoyancyDistanceAccumulator = 0.0f;
			BuoyancyDirection = 1.0f;
		}
		TargetLocation += FVector(0, 0, DeltaTime * BuoyancySpeed * BuoyancyDirection);
		break;
	}
	case EBuoyancyType::Rising:
	{
		TargetLocation += FVector(0, 0, BuoyancySpeed * DeltaTime);
		break;
	}
	case EBuoyancyType::Sinking:
	{
		TargetLocation -= FVector(0, 0, BuoyancySpeed * DeltaTime);
		break;
	}
	default:
		break;
	}
}

