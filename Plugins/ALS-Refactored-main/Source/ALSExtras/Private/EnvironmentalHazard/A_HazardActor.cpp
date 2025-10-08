#include "EnvironmentalHazard/A_HazardActor.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

AA_HazardActor::AA_HazardActor()
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

void AA_HazardActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!MeshDynamicMaterial)
	{
		MeshDynamicMaterial = StaticMeshComp->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (MeshDynamicMaterial)
	{
		MeshDynamicMaterial->SetScalarParameterValue("Damage", DamageCaused);
	}

	DefaultLocation = GetActorLocation();
}

void AA_HazardActor::BeginPlay()
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

	StaticMeshComp->OnComponentHit.AddDynamic(this, &AA_HazardActor::OnMeshHit);

	RandomAmplitude = FVector(BaseAmplitude * FMath::FRandRange(0.8f, 1.2f), BaseAmplitude * FMath::FRandRange(0.8f, 1.2f), BaseAmplitude * FMath::FRandRange(0.8f, 1.2f));

	RandomFrequency = FVector(BaseFrequency * FMath::FRandRange(0.8f, 1.2f), BaseFrequency * FMath::FRandRange(0.8f, 1.2f), BaseFrequency * FMath::FRandRange(0.8f, 1.2f));
}

void AA_HazardActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float Time = GetWorld()->GetTimeSeconds();
	const float Amplitude = 5.0f;
	const float Frequency = 0.5f;

	FVector Offset(FMath::Sin(Time * RandomFrequency.X) * RandomAmplitude.X, FMath::Cos(Time * RandomFrequency.Y * 0.7f) * RandomAmplitude.Y, FMath::Sin(Time * RandomFrequency.Z * 1.3f) * RandomAmplitude.Z * 0.5f);

	SetActorLocation(DefaultLocation + Offset);
}

void AA_HazardActor::HandleWeaponShot_Implementation(FHitResult& Hit)
{
	AudioComp->Play();
	--DamageCaused;

	if (DamageCaused >= 0)
	{
		MeshDynamicMaterial->SetScalarParameterValue("Damage", DamageCaused);
	}

	if (DamageCaused == 0)
	{
		OnDeath();
	}
}

void AA_HazardActor::OnDeath()
{
	AudioComp->SetSound(DeathSound);
	AudioComp->Play();
	DeathTimeline->PlayFromStart();
}

void AA_HazardActor::OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult)
{
	static bool bCanBeHit = true;

	if (!bCanBeHit || !OtherActor)
	{
		return;
	}

	bCanBeHit = false;

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, []()
		{
			bCanBeHit = true;
		}, 1.0f, false);

	UGameplayStatics::ApplyDamage(OtherActor, DamageCaused, nullptr, this, UDamageType::StaticClass());
}

void AA_HazardActor::DeathTimelineProgress(float Value)
{
	SetActorScale3D(FVector(1.0f - Value, 1.0f - Value, 1.0f - Value));
}

void AA_HazardActor::DeathTimelineFinished()
{
	Destroy();
}

