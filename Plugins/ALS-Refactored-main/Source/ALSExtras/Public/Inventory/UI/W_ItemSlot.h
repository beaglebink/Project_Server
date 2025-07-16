#pragma once

#include "Inventory/S_ItemData.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_ItemSlot.generated.h"

class UW_InventoryHUD;
class UTextBlock;
class UW_VisualDescription;

UCLASS()
class ALSEXTRAS_API UW_ItemSlot : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = "true"))
	FS_Item Item;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UW_InventoryHUD> InventoryHUDRef;

	UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (ExposeOnSpawn = "true"))
	EnumInventoryType InventoryType;

	UPROPERTY(BlueprintReadWrite, Category = "Slot text field", meta = (BindWidget))
	UTextBlock* TextBlock_Name;

	UPROPERTY(BlueprintReadWrite, Category = "Slot text field", meta = (BindWidget))
	UTextBlock* TextBlock_Quantity;

	UPROPERTY(BlueprintReadWrite, Category = "Slot text field", meta = (BindWidget))
	UTextBlock* TextBlock_Damage;

	UPROPERTY(BlueprintReadWrite, Category = "Slot text field", meta = (BindWidget))
	UTextBlock* TextBlock_Armor;

	UPROPERTY(BlueprintReadWrite, Category = "Slot text field", meta = (BindWidget))
	UTextBlock* TextBlock_Durability;

	UPROPERTY(BlueprintReadWrite, Category = "Slot text field", meta = (BindWidget))
	UTextBlock* TextBlock_Weight;

	UPROPERTY(BlueprintReadWrite, Category = "Slot text field", meta = (BindWidget))
	UTextBlock* TextBlock_Value;

	UPROPERTY(EditDefaultsOnly, Category = "ClassRefs")
	TSubclassOf<UW_VisualDescription> VisualDescriptionWidgetClass;

protected:
	UFUNCTION(BlueprintPure, Category = "Format")
	static FText FormatFloatFixed(float Value, int32 Precision);

	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)override;

	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent)override;
};
