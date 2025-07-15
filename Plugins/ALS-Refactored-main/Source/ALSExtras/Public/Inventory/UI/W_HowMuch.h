#pragma once

#include "Inventory/S_ItemData.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_HowMuch.generated.h"

class UW_InventoryHUD;
class UW_Inventory;
class UW_ItemSlot;
class USlider;
class UTextBlock;

UCLASS()
class ALSEXTRAS_API UW_HowMuch : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	USlider* Slider_HowMuch;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	UTextBlock* TextBlock_Value;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	UTextBlock* TextBlock_Max;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UW_InventoryHUD> InventoryHUDRef;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UW_Inventory> Inventory_FromRef;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UW_Inventory> Inventory_ToRef;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UW_ItemSlot> SlotRef;

	UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (ExposeOnSpawn = "true"))
	EnumInventoryType InventoryType;

	UPROPERTY(BlueprintReadWrite, Category = "Interaction", meta = (ExposeOnSpawn = "true"))
	uint8 bShouldSpawn : 1{false};
};
