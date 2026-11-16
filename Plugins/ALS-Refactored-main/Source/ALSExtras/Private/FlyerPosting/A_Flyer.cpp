#include "FlyerPosting/A_Flyer.h"
#include "Inventory/S_ItemData.h"

AA_Flyer::AA_Flyer()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	FlyerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlyerMesh"));

	FlyerMesh->SetupAttachment(RootComponent);

	FlyerMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FlyerMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	FlyerMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	FlyerMesh->SetWorldScale3D(FVector(1.0f, 0.2f, 0.25f));
}

void AA_Flyer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_Flyer::BeginPlay()
{
	Super::BeginPlay();

	float RandomRoll = FMath::RandRange(-10.0f, 10.0f);
	FlyerMesh->SetRelativeLocation(FVector(2.0f, 0.0f, 0.0f));
	FlyerMesh->AddLocalRotation(FRotator(0.0f, 0.0f, RandomRoll));

	if (FlyerTag != FGameplayTag::EmptyTag && IsValid(InventoryItemsDataTable))
	{
		MID_Flyer = FlyerMesh->CreateDynamicMaterialInstance(0);
		FlyerMesh->SetMaterial(0, MID_Flyer);

		TArray<FS_ItemData*> Rows;
		InventoryItemsDataTable->GetAllRows<FS_ItemData>(TEXT("Search"), Rows);

		for (FS_ItemData* Row : Rows)
		{
			if (FlyerTag.MatchesTag(Row->SpecialTag))
			{
				MID_Flyer->SetTextureParameterValue(TEXT("FlyerTexture"), Row->ImageDescription);
				break;
			}
		}
	}
}