#include "Inventory/UI/W_FullDescription.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Inventory/S_ItemData.h"

void UW_FullDescription::NativeConstruct()
{
	Super::NativeConstruct();

	if (ItemDataTable && Name != "")
	{
		Image_Description->SetBrushFromTexture(ItemDataTable->FindRow<FS_ItemData>(Name, TEXT("Find row in datatable"))->ImageDescription, true);

		TextBlock_Description->SetText(ItemDataTable->FindRow<FS_ItemData>(Name, TEXT("Find row in datatable"))->FullDescription);
	}
}
