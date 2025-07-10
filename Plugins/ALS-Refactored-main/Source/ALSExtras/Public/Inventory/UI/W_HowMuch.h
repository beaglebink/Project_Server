#pragma once

#include "Inventory/S_ItemData.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_HowMuch.generated.h"

class UW_InventoryHUD;

UCLASS()
class ALSEXTRAS_API UW_HowMuch : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = "true"))
	FName Name;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UW_InventoryHUD> InventoryHUDRef;

	UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (ExposeOnSpawn = "true"))
	EnumInventoryType InventoryType;
};
