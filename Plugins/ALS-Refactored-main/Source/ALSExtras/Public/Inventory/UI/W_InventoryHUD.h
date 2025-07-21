#pragma once

#include "Inventory/S_ItemData.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_InventoryHUD.generated.h"

struct FInputActionValue;
class UInputAction;
class UAC_Container;
class UW_Inventory;
class USizeBox;
class UW_ItemSlot;
class UW_HowMuch;
class UW_NotEnough;
class UButton;
class UVerticalBox;
class UCanvasPanel;

UCLASS()
class ALSEXTRAS_API UW_InventoryHUD : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items")
	UDataTable* ItemDataTable;

	UPROPERTY(BlueprintReadOnly, Category = "Refs", meta = (ExposeOnSpawn = "true"))
	EnumInventoryType InventoryType;

	UPROPERTY(BlueprintReadWrite, Category = "Refs", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UAC_Container> Container;

	UPROPERTY(BlueprintReadWrite, Category = "Refs")
	TObjectPtr<UW_Inventory> MainInventory;

	UPROPERTY(BlueprintReadWrite, Category = "Refs")
	TObjectPtr<UW_Inventory> AdditiveInventory;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	USizeBox* SizeBox_Main;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	USizeBox* SizeBox_Additive;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	USizeBox* SizeBox_VisualAndDescription;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	UButton* Button_TakeAll;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	UButton* Button_DropAll;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	UVerticalBox* VerticalBox_Additive;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	UCanvasPanel* CanvasPanel_Main;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	UCanvasPanel* CanvasPanel_Additive;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	UCanvasPanel* CanvasPanel_Confirmation_HowMuch_NotEnough;

	UPROPERTY(BlueprintReadWrite, Category = "KeyFlag")
	uint8 bHowMuchIsOpen : 1{false};

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UI")
	void Recreate();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Slot_OneClick(EnumInventoryType SlotInventoryType, UW_ItemSlot* SlotToMove, FName KeyPressed);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void CheckHowMuch(UW_Inventory* Inventory_From, UW_Inventory* Inventory_To, UW_ItemSlot* SlotToRemove, bool bShouldCount, bool bShouldSpawn = false);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void AddToSlotContainer(UW_Inventory* Inventory_To, UW_ItemSlot* SlotToAdd, int32 QuantityToAdd, bool bShouldCount);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void RemoveFromSlotContainer(UW_Inventory* Inventory_From, UW_ItemSlot* SlotToRemove, int32 QuantityToRemove, bool bShouldCount, bool bShouldSpawn = false);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UW_HowMuch> HowMuchWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UW_ItemSlot> SlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UW_NotEnough> NotEnoughWidgetClass;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "UI")
	UW_NotEnough* NotEnoughWidget;

	//input actions and methods
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	TObjectPtr<UInputAction> TakeAllAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	TObjectPtr<UInputAction> DropAllAction;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TakeAll();

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void DropAll();

private:
	float CurrentTradeCoeff = 1.0f;
};
