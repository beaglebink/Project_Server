#include "Inventory/AC_Inventory.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

UAC_Inventory::UAC_Inventory()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAC_Inventory::BeginPlay()
{
	Super::BeginPlay();

}

void UAC_Inventory::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAC_Inventory::BindInput(UEnhancedInputComponent* InputComponent)
{
	InputComponent->BindAction(InventoryAction, ETriggerEvent::Triggered, this, &UAC_Inventory::ToggleInventory);
}

void UAC_Inventory::ToggleInventory()
{
	if (!bIsOpen)
	{
		bIsOpen = true;

		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Green, "OPEN INVENTORY");
	}
	else
	{
		bIsOpen = false;

		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Red, "CLOSE INVENTORY");
	}
}

