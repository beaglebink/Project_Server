#pragma once

#include "S_ItemData.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AC_Inventory.generated.h"

struct FInputActionValue;
class UAC_Container;
class UInputAction;
class UInputMappingContext;
class UEnhancedInputLocalPlayerSubsystem;
class UEnhancedInputComponent;
class UW_InventoryHUD;
class UInteractiveItemComponent;

UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSEXTRAS_API UAC_Inventory : public UActorComponent
{
	GENERATED_BODY()

public:
	UAC_Inventory();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Items")
	TObjectPtr<UAC_Container> ContainerComponent;

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

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "IU")
	TSubclassOf<UW_InventoryHUD> InventoryClass;

	UPROPERTY(BlueprintReadWrite, Category = "IU")
	UW_InventoryHUD* Inventory;

	void BindInput(UEnhancedInputComponent* InputComponent);

private:
	uint8 bIsOpen : 1{false};

	TObjectPtr<UInteractiveItemComponent> CurrentInteractiveObject;

	void ToggleInventory();

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OpenInventory(EnumInventoryType SentInventoryType, UAC_Container* OtherContainer);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CloseInventory();
};
