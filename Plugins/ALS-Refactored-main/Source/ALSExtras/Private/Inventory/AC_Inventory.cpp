#include "Inventory/AC_Inventory.h"
#include "Inventory/AC_Container.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Inventory/UI/W_InventoryHUD.h"
#include "Kismet/GameplayStatics.h"
#include "AlsCharacterExample.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractivePickerComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractiveItemComponent.h"
#include "Inventory/I_OnInventoryClose.h"

UAC_Inventory::UAC_Inventory()
{
	PrimaryComponentTick.bCanEverTick = true;

	ContainerComponent = CreateDefaultSubobject<UAC_Container>(TEXT("ContainerComponent"));
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
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()->GetInstigatorController()))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		}
	}

	InputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &UAC_Inventory::ToggleInventory);
}

void UAC_Inventory::ToggleInventory()
{
	if (!bIsOpen)
	{
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.3f);
		OpenInventory(EnumInventoryType::Inventory, nullptr);
	}
	else
	{
		CloseInventory();
	}
}

void UAC_Inventory::OpenInventory(EnumInventoryType SentInventoryType, UAC_Container* OtherContainer)
{
	if (bIsOpen)
	{
		return;
	}

	bIsOpen = true;

	Subsystem->ClearAllMappings();
	Subsystem->AddMappingContext(Inventory_IMContext, 0);

	if (AAlsCharacterExample* Character = Cast<AAlsCharacterExample>(GetOwner()))
	{
		UInteractivePickerComponent* Picker = Cast< UInteractivePickerComponent>(Character->GetComponentByClass(UInteractivePickerComponent::StaticClass()));
		if (Picker && Picker->CurrentItem)
		{
			CurrentInteractiveObject = Picker->CurrentItem;
		}
	}

	if (InventoryClass)
	{
		Inventory = Cast<UW_InventoryHUD>(CreateWidget(GetWorld(), InventoryClass));
		Inventory->InventoryType = SentInventoryType;
		Inventory->Container = OtherContainer;
		Inventory->AddToViewport(11);

		APlayerController* PC = Cast<APlayerController>(GetOwner()->GetInstigatorController());
		if (PC)
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(Inventory->TakeWidget());
			PC->SetInputMode(InputMode);

			PC->SetIgnoreLookInput(true);
			PC->SetIgnoreMoveInput(true);
			PC->SetShowMouseCursor(true);
		}
	}
}

void UAC_Inventory::CloseInventory()
{
	if (!bIsOpen)
	{
		return;
	}

	bIsOpen = false;

	Subsystem->ClearAllMappings();
	AAlsCharacterExample* Character = Cast<AAlsCharacterExample>(GetOwner());
	Subsystem->AddMappingContext(Character->InputMappingContext, 0);

	if (Inventory)
	{
		Inventory->RemoveFromViewport();

		APlayerController* PC = Cast<APlayerController>(GetOwner()->GetInstigatorController());
		if (PC)
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->SetIgnoreLookInput(false);
			PC->SetIgnoreMoveInput(false);
			PC->SetShowMouseCursor(false);
			UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
		}
	}

	if (CurrentInteractiveObject)
	{
		if (CurrentInteractiveObject->GetOwner()->GetClass()->ImplementsInterface(UI_OnInventoryClose::StaticClass()))
		{
			II_OnInventoryClose::Execute_OnCloseInventoryEvent(CurrentInteractiveObject->GetOwner());
		}
	}

	UInteractivePickerComponent* Picker = Cast< UInteractivePickerComponent>(Character->GetComponentByClass(UInteractivePickerComponent::StaticClass()));
	if (Picker && Picker->CurrentItem)
	{
		Picker->CurrentItem = nullptr;
	}
}