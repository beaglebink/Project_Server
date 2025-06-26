#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AC_Inventory.generated.h"

class UInputAction;
class UInputMappingContext;

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

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	TObjectPtr<UInputMappingContext> Inventory_IMContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inputs")
	TObjectPtr<UInputAction> InventoryAction;

public:
	void BindInput(UEnhancedInputComponent* InputComponent);

private:
	uint8 bIsOpen : 1{false};

	void ToggleInventory();
};
