#pragma once

#include "S_ItemData.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AC_Inventory.generated.h"

struct FInputActionValue;
class UInputAction;
class UInputMappingContext;
class UEnhancedInputLocalPlayerSubsystem;
class UW_InventoryHUD;

UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSEXTRAS_API UAC_Inventory : public UActorComponent
{
	GENERATED_BODY()

public:
	UAC_Inventory();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UEnhancedInputLocalPlayerSubsystem* Subsystem;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	TObjectPtr<UInputMappingContext> Inventory_IMContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	TObjectPtr<UInputAction> InventoryAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	TObjectPtr<UInputAction> SurfAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	TObjectPtr<UInputAction> UseAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	TObjectPtr<UInputAction> DropAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Items")
	TArray<FS_Item> Items;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "IU")
	TSubclassOf<UW_InventoryHUD> InventoryClass;

	UPROPERTY(BlueprintReadWrite, Category = "IU")
	UW_InventoryHUD* Inventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items")
	UDataTable* ItemDataTable;

	void BindInput(UEnhancedInputComponent* InputComponent);

private:
	uint8 bIsOpen : 1{false};

	void ToggleInventory();

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OpenInventory(EnumInventoryType SentInventoryType = EnumInventoryType::Inventory, UAC_Container* Container = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CloseInventory();

private:
	void SurfInventory(const FInputActionValue& ActionValue);

	void UseInventory();

	void DropInventory();

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddToInventory(FName Name, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RemoveFromInventory(FName Name, int32 Quantity);
};
