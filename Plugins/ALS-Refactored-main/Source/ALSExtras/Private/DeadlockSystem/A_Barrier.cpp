#include "DeadlockSystem/A_Barrier.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"

AA_Barrier::AA_Barrier()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));

	MeshComponent->SetupAttachment(RootComponent);
	BoxComponent->SetupAttachment(MeshComponent);
	AudioComponent->SetupAttachment(RootComponent);

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	BoxComponent->SetBoxExtent(FVector(100.f, 50.f, 50.f));
	BoxComponent->SetGenerateOverlapEvents(true);
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoxComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void AA_Barrier::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!MaterialInstanceDynamic)
	{
		MaterialInstanceDynamic = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (MaterialInstanceDynamic)
	{
		MaterialInstanceDynamic->SetScalarParameterValue("IsOpen", 0.0f);
	}
}

void AA_Barrier::BeginPlay()
{
	Super::BeginPlay();

	BoxComponent->OnComponentBeginOverlap.AddDynamic(this, &AA_Barrier::OnBarrierBeginOverlap);
	BoxComponent->OnComponentEndOverlap.AddDynamic(this, &AA_Barrier::OnBarrierEndOverlap);
}

void AA_Barrier::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsOverlapping)
	{
		MaterialInstanceDynamic->SetVectorParameterValue("ActorLocation", OverlappingActor->GetActorLocation());
	}
}

void AA_Barrier::OnBarrierBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	bIsOverlapping = true;
	OverlappingActor = OtherActor;
}

void AA_Barrier::OnBarrierEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	bIsOverlapping = false;
}

void AA_Barrier::GetTextCommand(FText Command)
{
	if (Command.EqualTo(BarrierPassword))
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MaterialInstanceDynamic->SetScalarParameterValue("IsOpen", 1.0f);

		if (AudioComponent && AudioComponent->IsPlaying())
		{
			AudioComponent->Stop();
		}
	}
}

void AA_Barrier::SetBarrierPassword(const FText& NewPassword)
{
	BarrierPassword = NewPassword;
}

FText AA_Barrier::GetBarrierPassword() const
{
	return BarrierPassword;
}

