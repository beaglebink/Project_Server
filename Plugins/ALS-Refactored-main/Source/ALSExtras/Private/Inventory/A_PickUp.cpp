#include "Inventory/A_PickUp.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractiveItemComponent.h"

AA_PickUp::AA_PickUp()
{
	PrimaryActorTick.bCanEverTick = false;

	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	InteractiveComponent = CreateDefaultSubobject<UInteractiveItemComponent>(TEXT("InteractiveComponent"));
}

FS_ItemData AA_PickUp::GetItemData() const
{
	if (ItemDataTable)
	{
		const FS_ItemData* FoundRow = ItemDataTable->FindRow<FS_ItemData>(Item.Name, TEXT("GetItemData"));
		if (FoundRow)
		{
			return *FoundRow;
		}
	}
	return FS_ItemData();
}

void AA_PickUp::BeginPlay()
{
	Super::BeginPlay();

	Item.Quantity = 1;
}
