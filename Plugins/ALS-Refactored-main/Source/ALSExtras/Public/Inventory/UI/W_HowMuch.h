#pragma once

#include "Inventory/S_ItemData.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_HowMuch.generated.h"

struct FInputActionValue;
class UInputAction;
class UW_InventoryHUD;
class UW_Inventory;
class UW_ItemSlot;
class USlider;
class UTextBlock;
class UW_FocusableButton;

UCLASS()
class ALSEXTRAS_API UW_HowMuch : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	virtual void NativeDestruct() override;

public:
	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	USlider* Slider_HowMuch;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	UTextBlock* TextBlock_Value;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	UTextBlock* TextBlock_Max;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	UW_FocusableButton* Button_Yes;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	UW_FocusableButton* Button_No;

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
	uint8 bShouldCount : 1{false};

	UPROPERTY(BlueprintReadWrite, Category = "Interaction", meta = (ExposeOnSpawn = "true"))
	uint8 bShouldSpawn : 1{false};

	//input actions and methods
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	TObjectPtr<UInputAction> SurfAction;

	void SurfSlider(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void ConfirmSlider();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void EscapeWidget();
};
