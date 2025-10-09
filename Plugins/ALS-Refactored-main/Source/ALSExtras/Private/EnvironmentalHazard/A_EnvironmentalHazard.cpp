#include "EnvironmentalHazard/A_EnvironmentalHazard.h"
#include "Components/AudioComponent.h"

AA_EnvironmentalHazard::AA_EnvironmentalHazard()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	DeathTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DeathTimeline"));

	StaticMeshComp->SetupAttachment(RootComponent);
	AudioComp->SetupAttachment(RootComponent);

	StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	StaticMeshComp->SetCollisionObjectType(ECC_WorldDynamic);
	StaticMeshComp->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	StaticMeshComp->SetNotifyRigidBodyCollision(true);

	AudioComp->bAutoActivate = false;
	AudioComp->SetSound(HitSound);
}

void AA_EnvironmentalHazard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	DefaultLocation = GetActorLocation();
}

void AA_EnvironmentalHazard::BeginPlay()
{
	Super::BeginPlay();

	if (DeathFloatCurve)
	{
		DeathProgressFunction.BindUFunction(this, FName("DeathTimelineProgress"));
		DeathTimeline->AddInterpFloat(DeathFloatCurve, DeathProgressFunction);

		DeathFinishedFunction.BindUFunction(this, FName("DeathTimelineFinished"));
		DeathTimeline->SetTimelineFinishedFunc(DeathFinishedFunction);

		DeathTimeline->SetLooping(false);
	}

	StaticMeshComp->OnComponentHit.AddDynamic(this, &AA_EnvironmentalHazard::OnMeshHit);

	RandomAmplitude = FVector(BaseAmplitude * FMath::FRandRange(0.8f, 1.2f), BaseAmplitude * FMath::FRandRange(0.8f, 1.2f), BaseAmplitude * FMath::FRandRange(0.8f, 1.2f));

	RandomFrequency = FVector(BaseFrequency * FMath::FRandRange(0.8f, 1.2f), BaseFrequency * FMath::FRandRange(0.8f, 1.2f), BaseFrequency * FMath::FRandRange(0.8f, 1.2f));
}

void AA_EnvironmentalHazard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FloatingWave();
}

void AA_EnvironmentalHazard::OnDeath()
{
	bIsOnDeath = true;
	AudioComp->SetSound(DeathSound);
	AudioComp->Play();
	DeathTimeline->PlayFromStart();
}

void AA_EnvironmentalHazard::OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult)
{
	if (!bCanBeHit || bIsOnDeath || !OtherActor)
	{
		return;
	}

	bCanBeHit = false;

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			bCanBeHit = true;
		}, 1.0f, false);
}

void AA_EnvironmentalHazard::FloatingWave()
{
	const float Time = GetWorld()->GetTimeSeconds();
	const float Amplitude = 5.0f;
	const float Frequency = 0.5f;

	FVector Offset(FMath::Sin(Time * RandomFrequency.X) * RandomAmplitude.X, FMath::Cos(Time * RandomFrequency.Y * 0.7f) * RandomAmplitude.Y, FMath::Sin(Time * RandomFrequency.Z * 1.3f) * RandomAmplitude.Z * 0.5f);

	SetActorLocation(DefaultLocation + Offset);
}

void AA_EnvironmentalHazard::DeathTimelineProgress(float Value)
{
	SetActorScale3D(FVector(1.0f - Value, 1.0f - Value, 1.0f - Value));
}

void AA_EnvironmentalHazard::DeathTimelineFinished()
{
	Destroy();
}

