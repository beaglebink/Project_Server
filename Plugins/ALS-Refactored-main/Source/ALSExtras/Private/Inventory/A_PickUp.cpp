#include "Inventory/A_PickUp.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractiveItemComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractivePickerComponent.h"
#include "Inventory/AC_Inventory.h"
#include "Inventory/AC_Container.h"
#include "FloorAssignmentComponent.h"
#include "Kismet/GameplayStatics.h"


AA_PickUp::AA_PickUp()
{
	PrimaryActorTick.bCanEverTick = false;

	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	InteractiveComponent = CreateDefaultSubobject<UInteractiveItemComponent>(TEXT("InteractiveComponent"));
	FloorAssignmentComponent = CreateDefaultSubobject<UFloorAssignmentComponent>(TEXT("FloorAssignmentComponent"));
	RootComponent = StaticMeshComp;

	StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	StaticMeshComp->SetCollisionProfileName(TEXT("PhysicsActor"));
	StaticMeshComp->SetSimulatePhysics(true);
}

void AA_PickUp::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGIS_PickUp* GIS = GI->GetSubsystem<UGIS_PickUp>())
		{
			GIS->AddSaveableObject(this);

			// Delay the application of chore placement data to the next tick to ensure that all actors are properly initialized
			GetWorldTimerManager().SetTimerForNextTick([this, GIS]()
				{
					ApplyPickUpSaveData_Implementation(GIS);
				});
		}
	}

	InteractiveComponent->OnInteractionPressKeyEvent.AddDynamic(this, &AA_PickUp::AddToInventory);
	InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to pick up"));
}

void AA_PickUp::AddToInventory(UInteractivePickerComponent* Picker)
{
	UAC_Inventory* Inventory = Cast<UAC_Inventory>(Picker->GetOwner()->GetComponentByClass(UAC_Inventory::StaticClass()));

	if (Inventory->bIsOpen)
	{
		return;
	}

	Inventory->ContainerComponent->AddToContainer(Name, 1, 1.0f, false);

	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}

	Destroy();
}

FString AA_PickUp::GetFloorAssignmentComponentIdAsString_Implementation()
{
	if (IsValid(FloorAssignmentComponent))
	{
		return FloorAssignmentComponent->GetFloorId().ToString();
	}
	return FString();
}

FPickUpSaveData AA_PickUp::CollectPickUpSaveData_Implementation()
{
	FPickUpSaveData PickUpSaveData;
	PickUpSaveData.PickUpTransform = GetActorTransform();

	return PickUpSaveData;
}

void AA_PickUp::ApplyPickUpSaveData_Implementation(UGIS_PickUp* GIS)
{
	FPickUpSaveData* PickUpSaveData = GIS->CachedPickUpData.Find(GetFloorAssignmentComponentIdAsString_Implementation());
	if (PickUpSaveData)
	{
		SetActorTransform(PickUpSaveData->PickUpTransform);
	}
}

FString AA_PickUp::CollectIntermediateSaveData()
{
	FSubsystemSaveData SaveData;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGIS_PickUp* GIS = GI->GetSubsystem<UGIS_PickUp>())
		{
			GIS->CollectSaveData(SaveData);
		}
	}
	return SaveData.SerializedData;
}

void AA_PickUp::ApplyIntermediateSaveData(FString Jsonstring)
{
	FSubsystemSaveData SaveData;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGIS_PickUp* GIS = GI->GetSubsystem<UGIS_PickUp>())
		{
			SaveData.SerializedData = Jsonstring;
			GIS->ApplySaveData(SaveData);
			ApplyPickUpSaveData_Implementation(GIS);
		}
	}
}