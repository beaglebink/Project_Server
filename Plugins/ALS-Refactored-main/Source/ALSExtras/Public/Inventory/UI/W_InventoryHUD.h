#pragma once

#include "Inventory/S_ItemData.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_InventoryHUD.generated.h"

class UAC_Container;
class UW_Inventory;

UCLASS()
class ALSEXTRAS_API UW_InventoryHUD : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (ExposeOnSpawn = "true"))
	EnumInventoryType InventoryType;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UAC_Container> Container;

	UPROPERTY(BlueprintReadWrite, Category = "Refs")
	TObjectPtr<UW_Inventory> MainInventory;

	UPROPERTY(BlueprintReadWrite, Category = "Refs")
	TObjectPtr<UW_Inventory> AdditiveInventory;
};
