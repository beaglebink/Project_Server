#include "BookAndWords/A_Book.h"

AA_Book::AA_Book()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));

	StaticMeshComponent->SetupAttachment(RootComponent);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	StaticMeshComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	StaticMeshComponent->SetCollisionProfileName("OverlapAllDynamic");
	StaticMeshComponent->SetCollisionResponseToAllChannels(ECR_Overlap);
}

void AA_Book::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AA_Book::BeginPlay()
{
	Super::BeginPlay();
}

void AA_Book::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_Book::AddWord(FText NewWord)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, FString::Printf(TEXT("%s"), *NewWord.ToString()));
}
