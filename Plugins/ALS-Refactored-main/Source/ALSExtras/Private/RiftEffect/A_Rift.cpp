#include "RiftEffect/A_Rift.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"

AA_Rift::AA_Rift()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComp"));

	NiagaraComp->SetupAttachment(RootComponent);
}

void AA_Rift::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	for (size_t i = 0; i < FromLeftBoxes.Num(); ++i)
	{
		if (FromLeftBoxes[i].BoxComponent && FromRightBoxes[i].BoxComponent)
		{
			FromLeftBoxes[i].BoxComponent->DestroyComponent();
			FromRightBoxes[i].BoxComponent->DestroyComponent();
		}
	}

	FromLeftBoxes.Empty();
	FromRightBoxes.Empty();

	if (!NiagaraComp)
	{
		return;
	}

	FBoxSphereBounds NiagaraBounds = NiagaraComp->Bounds;
	FVector Extent = NiagaraBounds.BoxExtent * 2.0f;

	int32 LoopNum = Extent.Z / BoxSize.Z - 1;
	float SpaceBetweenBoxes = Extent.Y / 2.0f + BoxSize.Y / 2;
	uint8 bIsLeft = true;

	for (int32 i = 0; i < LoopNum; ++i)
	{
		FString LeftName = FString::Printf(TEXT("LeftBox_%d"), i);
		UBoxComponent* BoxCompLeft = NewObject<UBoxComponent>(this, *LeftName);
		BoxCompLeft->SetupAttachment(RootComponent);
		BoxCompLeft->RegisterComponent();
		BoxCompLeft->SetBoxExtent(BoxSize * 0.5f);
		BoxCompLeft->SetRelativeLocation(FVector(0.0f, SpaceBetweenBoxes * (static_cast<int32>(bIsLeft) * 2 - 1), BoxSize.Z * i));
		BoxCompLeft->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		BoxCompLeft->SetCollisionObjectType(ECC_WorldDynamic);
		BoxCompLeft->OnComponentHit.AddDynamic(this, &AA_Rift::OnBoxHit);



		FString RightName = FString::Printf(TEXT("RightBox_%d"), i);
		UBoxComponent* BoxCompRight = NewObject<UBoxComponent>(this, *RightName);
		BoxCompRight->SetupAttachment(RootComponent);
		BoxCompRight->RegisterComponent();
		BoxCompRight->SetBoxExtent(BoxSize * 0.5f);
		BoxCompRight->SetRelativeLocation(FVector(0.0f, SpaceBetweenBoxes * (static_cast<int32>(!bIsLeft) * 2 - 1), BoxSize.Z * i));
		BoxCompRight->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		BoxCompRight->SetCollisionObjectType(ECC_WorldDynamic);
		BoxCompRight->OnComponentHit.AddDynamic(this, &AA_Rift::OnBoxHit);


		FRiftBoxData DataLeft;
		DataLeft.BoxComponent = BoxCompLeft;
		DataLeft.Index = i;
		DataLeft.Side = ERiftSide::Left;

		FRiftBoxData DataRight;
		DataRight.BoxComponent = BoxCompRight;
		DataRight.Index = i;
		DataRight.Side = ERiftSide::Right;

		FromLeftBoxes.Add(DataLeft);
		FromRightBoxes.Add(DataRight);
	}
	bIsLeft = !bIsLeft;
}

void AA_Rift::BeginPlay()
{
	Super::BeginPlay();
}

void AA_Rift::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_Rift::OnBoxHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this)
		return;

	UE_LOG(LogTemp, Warning, TEXT("Hit detected! Box: %s | Other: %s"),
		*HitComp->GetName(), *OtherActor->GetName());

}

