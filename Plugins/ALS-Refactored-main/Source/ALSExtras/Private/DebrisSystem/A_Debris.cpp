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
	InitialDirection = FMath::VRand();
	NoiseSeed = FMath::FRandRange(0.0f, 10.0f);
	OrbitRadius = FMath::FRandRange(20.0f, 100.0f);
	OrbitStartOffset = FMath::VRand() * OrbitRadius;

	//Initialize player reaction variables

}

void AA_Debris::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DebrisFloatBehavior(DeltaTime);

	DebrisReactToPlayer(DeltaTime);
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
		FVector Chaos;
		Chaos.X = FMath::PerlinNoise1D(TimeAccumulator * 3.0f + NoiseSeed);
		Chaos.Y = FMath::PerlinNoise1D(TimeAccumulator * 4.0f + NoiseSeed * 2);
		Chaos.Z = FMath::PerlinNoise1D(TimeAccumulator * 5.0f);

		SetActorLocation(GetActorLocation() + Chaos * DebrisMovementSpeed * DeltaTime * 30.0f);
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

	FVector SwirlAxis = FVector::CrossProduct(PlayerVelocity.GetSafeNormal(), FVector::UpVector);
	FVector Swirl = SwirlAxis.GetSafeNormal() * TurbulenceStrength * InfluenceAlpha * FMath::Sin(GetWorld()->TimeSeconds * 3.0f);

	switch (PlayerReaction)
	{
	case EDebrisReactToPlayer::Passive:
		break;
	case EDebrisReactToPlayer::Evasive:
	{
		FVector Evasion = -DirToPlayer * PlayerInfluenceStrength * InfluenceAlpha;
		SetActorLocation(GetActorLocation() + Evasion);
		break;
	}
	case EDebrisReactToPlayer::Attracted:
	{
		FVector Attracion = DirToPlayer * PlayerInfluenceStrength * InfluenceAlpha;
		SetActorLocation(GetActorLocation() + Attracion);
		break;
	}
	case EDebrisReactToPlayer::Neutral:
	{
		FVector FlowInfluence = (PlayerVelocity / 100.0f + DirToPlayer) * PlayerInfluenceStrength * InfluenceAlpha;
		SetActorLocation(FMath::VInterpTo(GetActorLocation(), GetActorLocation() + FlowInfluence + Swirl, GetWorld()->DeltaTimeSeconds, 10.0f));
		break;
	}
	default:
		break;
	}
}

