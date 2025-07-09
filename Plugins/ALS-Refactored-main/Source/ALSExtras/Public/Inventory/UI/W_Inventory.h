#pragma once

#include "Inventory/S_ItemData.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_Inventory.generated.h"

class UW_InventoryHUD;
class UAC_Container;
class UW_SlotContainer;

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

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	TObjectPtr<UW_SlotContainer> SlotContainer_All;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	TObjectPtr<UW_SlotContainer> SlotContainer_Weapon;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	TObjectPtr<UW_SlotContainer> SlotContainer_Clothes;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	TObjectPtr<UW_SlotContainer> SlotContainer_Consum;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	TObjectPtr<UW_SlotContainer> SlotContainer_Misc;

	UPROPERTY(BlueprintReadWrite, Category = "UI")
	TObjectPtr<UW_SlotContainer> SlotContainer_Other;
};
