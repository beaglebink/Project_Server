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
		if (IsValid(FromLeftBoxes[i]) && IsValid(FromRightBoxes[i]))
		{
			FromLeftBoxes[i]->DestroyComponent();
			FromRightBoxes[i]->DestroyComponent();
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
		BoxCompLeft->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		BoxCompLeft->SetCollisionObjectType(ECC_WorldDynamic);
		BoxCompLeft->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		BoxCompLeft->bHiddenInGame = false;

		FString RightName = FString::Printf(TEXT("RightBox_%d"), i);
		UBoxComponent* BoxCompRight = NewObject<UBoxComponent>(this, *RightName);
		BoxCompRight->SetupAttachment(RootComponent);
		BoxCompRight->RegisterComponent();
		BoxCompRight->SetBoxExtent(BoxSize * 0.5f);
		BoxCompRight->SetRelativeLocation(FVector(0.0f, SpaceBetweenBoxes * (static_cast<int32>(!bIsLeft) * 2 - 1), BoxSize.Z * i));
		BoxCompRight->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		BoxCompRight->SetCollisionObjectType(ECC_WorldDynamic);
		BoxCompRight->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		BoxCompRight->bHiddenInGame = false;

		FromLeftBoxes.Add(BoxCompLeft);
		FromRightBoxes.Add(BoxCompRight);

		bIsLeft = !bIsLeft;
	}
}

void AA_Rift::BeginPlay()
{
	Super::BeginPlay();
}

void AA_Rift::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_Rift::HandleWeaponShot_Implementation(FHitResult& Hit)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("%s"), *Hit.GetComponent()->GetName()));
}

void AA_Rift::HandleTextFromWeapon_Implementation(const FText& TextCommand)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Rift Text Command: %s"), *TextCommand.ToString()));
}
