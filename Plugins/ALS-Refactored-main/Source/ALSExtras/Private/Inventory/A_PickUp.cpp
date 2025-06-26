#include "Inventory/A_PickUp.h"

AA_PickUp::AA_PickUp()
{
	PrimaryActorTick.bCanEverTick = true;

}

const FS_ItemData AA_PickUp::GetItemData() const
{
    if (ItemDataTable)
    {
        const FS_ItemData* FoundRow = ItemDataTable->FindRow<FS_ItemData>(ItemID, TEXT("GetItemData"));
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
	
}

void AA_PickUp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

