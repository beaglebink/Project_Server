#pragma once

#include "CoreMinimal.h"
#include "Inventory/AC_Container.h"
#include "Blueprint/UserWidget.h"
#include "W_ItemSlot.generated.h"

class UTextBlock;

UCLASS()
class ALSEXTRAS_API UW_ItemSlot : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UAC_Container> OtherContainer;

public:
	UPROPERTY(BlueprintReadWrite, Category = "Slot text field", meta = (BindWidget))
	UTextBlock* TextBlock_Name;

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

protected:
	UFUNCTION(BlueprintPure, Category = "Format")
	static FText FormatFloatFixed(float Value, int32 Precision);
};
