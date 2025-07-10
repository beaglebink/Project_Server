#pragma once

#include "Inventory/S_ItemData.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_Inventory.generated.h"

class UW_InventoryHUD;
class UAC_Container;
class UW_SlotContainer;
class UW_ItemSlot;
class UScrollBox;

UCLASS()
class ALSEXTRAS_API UW_Inventory : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UW_InventoryHUD> InventoryHUDRef;

	UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (ExposeOnSpawn = "true"))
	EnumInventoryType InventoryType;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UAC_Container> Container;

	UPROPERTY(BlueprintReadWrite, Category = "Items")
	TArray<TObjectPtr<UW_ItemSlot>> Slots;

	UPROPERTY(BlueprintReadWrite, Category = "ScrollBox field", meta = (BindWidget))
	UScrollBox* ScrollBox_Items;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Items")
	UDataTable* ItemDataTable;

	UFUNCTION(BlueprintCallable, Category = "Filter")
	void SlotsFilter(EnumInventory SlotContainerType);

	//sorting
private:
	uint8 bIsDecreased : 1{false};

	EnumSortType PrevSort;

protected:
	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void Items_Sort(EnumSortType SortType, EnumInventory SlotContainerType);
};
