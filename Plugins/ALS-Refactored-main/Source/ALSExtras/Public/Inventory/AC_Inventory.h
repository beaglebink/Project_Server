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
class UW_CharacterUI;

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
	UPROPERTY(SaveGame)
	UEnhancedInputLocalPlayerSubsystem* Subsystem;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, SaveGame, Category = "Inputs")
	TObjectPtr<UInputMappingContext> Inventory_IMContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, SaveGame, Category = "Inputs")
	TObjectPtr<UInputAction> InventoryAction;

public:
	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "IU")
	TSubclassOf<UW_InventoryHUD> InventoryClass;

	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "IU")
	UW_InventoryHUD* Inventory;

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "IU")
	TSubclassOf<UW_CharacterUI> CharacterWidgetClass;
	
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "IU")
	UW_CharacterUI* CharacterWidget;

	void BindInput(UEnhancedInputComponent* InputComponent);

	UPROPERTY(SaveGame)
	uint8 bIsOpen : 1{false};

private:
	UPROPERTY(SaveGame)
	TObjectPtr<UInteractiveItemComponent> CurrentInteractiveObject;

	void ToggleInventory();

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OpenInventory(EnumInventoryType SentInventoryType, UAC_Container* OtherContainer);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void CloseInventory();
};
