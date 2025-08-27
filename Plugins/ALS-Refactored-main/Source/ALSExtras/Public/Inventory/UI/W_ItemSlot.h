#pragma once

#include "Inventory/S_ItemData.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_ItemSlot.generated.h"

class UW_InventoryHUD;
class UTextBlock;
class UImage;
class UW_VisualDescription;
class UW_FocusableButton;
class UInputAction;

UCLASS()
class ALSEXTRAS_API UW_ItemSlot : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativePreConstruct() override;

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

	UPROPERTY(BlueprintReadWrite, Category = "Slot Image field", meta = (BindWidget))
	UImage* Image_Background;
	
	UPROPERTY(BlueprintReadOnly, Category = "Button", meta = (BindWidget))
	UW_FocusableButton* Button_Description;

	UPROPERTY(EditDefaultsOnly, Category = "ClassRefs")
	TSubclassOf<UW_VisualDescription> VisualDescriptionWidgetClass;

	UFUNCTION(BlueprintPure, Category = "Format")
	static FText FormatFloatFixed(float Value, int32 Precision);

public:
	void SetTintOnSelected(bool IsSet);

protected:
	UFUNCTION(BlueprintCallable)
	void FullDescriptionCreate();

	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)override;

	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent)override;

	virtual FReply NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)override;

	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent)override;
};
