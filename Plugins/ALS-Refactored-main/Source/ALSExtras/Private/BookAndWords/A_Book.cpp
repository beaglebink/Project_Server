#include "BookAndWords/A_Book.h"
#include "BookAndWords/C_Word.h"

AA_Book::AA_Book()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));

	StaticMeshComponent->SetupAttachment(RootComponent);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	StaticMeshComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	StaticMeshComponent->SetCollisionProfileName("OverlapAllDynamic");
	StaticMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
}

void AA_Book::OnCostruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AA_Book::BeginPlay()
{
	Super::BeginPlay();

	StaticMeshComponent->OnComponentBeginOverlap.AddDynamic(this, &AA_Book::OnMeshBeginOverlap);
}

void AA_Book::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_Book::OnMeshBeginOverlap(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}

	if (AC_Word* Word = Cast<AC_Word>(OtherActor))
	{
		if (Word->BookGroupCode == BookGroupCode)
		{
			Word->Absorbing();
		}
	}
}

