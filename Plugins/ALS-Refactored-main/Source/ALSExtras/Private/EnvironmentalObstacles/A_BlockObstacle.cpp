#include "EnvironmentalObstacles/A_BlockObstacle.h"
#include "EnvironmentalObstacles/A_BlocksWallObstacle.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"

AA_BlockObstacle::AA_BlockObstacle()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	BlockMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlockMeshComponent"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	BlockDestroyFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BlockDestroyFXComponent"));
	DestroyTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DestroyTimelineComponent"));
	MoveBlockTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("MoveTimelineComponent"));
	FallBlockTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("FallTimelineComponent"));

	BlockMeshComponent->SetupAttachment(RootComponent);
	AudioComponent->SetupAttachment(RootComponent);
	BlockDestroyFX->SetupAttachment(BlockMeshComponent);

	AudioComponent->bAutoActivate = false;
	BlockDestroyFX->bAutoActivate = false;
}

void AA_BlockObstacle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (BlockMaterialAsset && !IsValid(BlockMaterialInstance))
	{
		BlockMaterialInstance = UMaterialInstanceDynamic::Create(BlockMaterialAsset, BlockMeshComponent);
		BlockMaterialInstance->SetScalarParameterValue(FName("IsCritical"), 0);
		BlockMeshComponent->SetMaterial(0, BlockMaterialInstance);
	}
}

void AA_BlockObstacle::BeginPlay()
{
	Super::BeginPlay();

	//Destroy timeline
	if (DestroyFloatCurve)
	{
		DestroyProgressFunction.BindUFunction(this, FName("DestroyTimelineProgress"));
		DestroyTimeline->AddInterpFloat(DestroyFloatCurve, DestroyProgressFunction);

		DestroyFinishedFunction.BindUFunction(this, FName("DestroyTimelineFinished"));
		DestroyTimeline->SetTimelineFinishedFunc(DestroyFinishedFunction);

		DestroyTimeline->SetLooping(false);
	}

	//Move timeline 
	if (MoveBlockFloatCurve)
	{
		MoveBlockProgressFunction.BindUFunction(this, FName("MoveBlockTimelineProgress"));
		MoveBlockTimeline->AddInterpFloat(MoveBlockFloatCurve, MoveBlockProgressFunction);

		MoveBlockFinishedFunction.BindUFunction(this, FName("MoveBlockTimelineFinished"));
		MoveBlockTimeline->SetTimelineFinishedFunc(MoveBlockFinishedFunction);

		MoveBlockTimeline->SetLooping(false);
	}

	//Fall timeline 
	if (FallBlockFloatCurve)
	{
		FallBlockProgressFunction.BindUFunction(this, FName("FallBlockTimelineProgress"));
		FallBlockTimeline->AddInterpFloat(FallBlockFloatCurve, FallBlockProgressFunction);

		FallBlockFinishedFunction.BindUFunction(this, FName("FallBlockTimelineFinished"));
		FallBlockTimeline->SetTimelineFinishedFunc(FallBlockFinishedFunction);

		FallBlockTimeline->SetLooping(false);
	}
}

void AA_BlockObstacle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_BlockObstacle::SetBlockState(EBlockState NewState)
{
	switch (NewState)
	{
	case EBlockState::Neutral:
	{
		BlockState = EBlockState::Neutral;
		SetBlockMaterial(false);
		break;
	}
	case EBlockState::Critical:
	{
		BlockState = EBlockState::Critical;
		SetBlockMaterial(true);
		break;
	}
	case EBlockState::Destroyed:
	{
		BlockState = EBlockState::Destroyed;
		StartBlockDestroy();
		break;
	}
	case EBlockState::Moving:
	{
		BlockState = EBlockState::Moving;
		MoveBlock(EDirection::Down);
		break;
	}
	case EBlockState::Falling:
	{
		BlockState = EBlockState::Falling;
		StartBlockFall();
		break;
	}
	case EBlockState::Shot:
	{
		BlockState = EBlockState::Shot;
		OnShotMaterial(true);
		FTimerHandle ResetShotHandle;
		GetWorldTimerManager().SetTimer(ResetShotHandle, [this]()
			{
				OnShotMaterial(false);
				BlockState = EBlockState::Neutral;
			}, 0.2f, false);
		break;
	}
	default:
		break;
	}
}

void AA_BlockObstacle::SetBlockMaterial(bool bIsCritical)
{
	BlockMaterialInstance->SetScalarParameterValue(FName("IsCritical"), static_cast<int32>(bIsCritical));
}

void AA_BlockObstacle::OnShotMaterial(bool bIsShot)
{
	BlockMaterialInstance->SetScalarParameterValue(FName("EmissiveIntensity"), 1 + static_cast<int32>(bIsShot) * 10);
}

void AA_BlockObstacle::StartBlockDestroy()
{
	UMaterialInstanceDynamic* DynMat = nullptr;
	DynMat = UMaterialInstanceDynamic::Create(BlockMaterialAsset, BlockDestroyFX);
	DynMat->SetScalarParameterValue(FName(TEXT("IsNiagaraMaterial")), 1);
	DynMat->SetScalarParameterValue(FName(TEXT("IsCritical")), 1);
	BlockDestroyFX->SetVariableMaterial(FName(TEXT("User.ParticlesMaterial")), DynMat);
	BlockDestroyFX->Activate(true);

	AudioComponent->SetSound(BlockDestroySound);
	AudioComponent->Play();
	DestroyTimeline->PlayFromStart();
}

void AA_BlockObstacle::MoveBlock(EDirection Direction)
{

}

void AA_BlockObstacle::StartBlockFall()
{
	AudioComponent->SetSound(BlockFallSound);
	FallBlockTimeline->PlayFromStart();
}

void AA_BlockObstacle::HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit)
{
	if (BlockState == EBlockState::Critical)
	{
		SetBlockState(EBlockState::Destroyed);
	}
	else if (BlockState == EBlockState::Neutral)
	{
		SetBlockState(EBlockState::Shot);
	}
}

void AA_BlockObstacle::DestroyTimelineProgress(float Value)
{
	BlockMaterialInstance->SetScalarParameterValue(FName(TEXT("Opacity")), Value);
}
void AA_BlockObstacle::DestroyTimelineFinished()
{
	AudioComponent->Stop();

	if (AA_BlocksWallObstacle* GridOfBlocks = Cast<AA_BlocksWallObstacle>(GetOwner()))
	{
		GridOfBlocks->UpperBlocksFall(BlockIndex);
	}
}

void AA_BlockObstacle::MoveBlockTimelineProgress(float Value)
{
}

void AA_BlockObstacle::MoveBlockTimelineFinished()
{
	AudioComponent->Stop();
}

void AA_BlockObstacle::FallBlockTimelineProgress(float Value)
{
	SetActorLocation(FMath::Lerp(InitialLocation, TargetLocation, Value));
}

void AA_BlockObstacle::FallBlockTimelineFinished()
{
	InitialLocation = TargetLocation;
	AudioComponent->Play();
}

