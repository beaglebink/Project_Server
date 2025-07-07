#include "Inventory/AC_Inventory.h"
#include "Inventory/A_PickUp.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Inventory/UI/W_InventoryHUD.h"
#include "Kismet/GameplayStatics.h"
#include "AlsCharacterExample.h"
#include "Inventory/AC_Container.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractivePickerComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractiveItemComponent.h"
#include "Inventory/I_OnInventoryClose.h"

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
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()->GetInstigatorController()))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		}
	}

	InputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &UAC_Inventory::ToggleInventory);
	InputComponent->BindAction(SurfAction, ETriggerEvent::Triggered, this, &UAC_Inventory::SurfInventory);
	InputComponent->BindAction(UseAction, ETriggerEvent::Triggered, this, &UAC_Inventory::UseInventory);
	InputComponent->BindAction(DropAction, ETriggerEvent::Triggered, this, &UAC_Inventory::DropInventory);
}

void UAC_Inventory::ToggleInventory()
{
	if (!bIsOpen)
	{
		OpenInventory();
	}
	else
	{
		CloseInventory();
	}
}

void UAC_Inventory::OpenInventory(EnumInventoryType SentInventoryType, UAC_Container* Container)
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
		Inventory->OtherContainer = Container;
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
			UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.0f);
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

void UAC_Inventory::SurfInventory(const FInputActionValue& ActionValue)
{
	FVector2D Value = ActionValue.Get<FVector2D>();

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Green, FString::Printf(TEXT("SURF %2.2f"), Value.X));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Red, FString::Printf(TEXT("SURF %2.2f"), Value.Y));
}

void UAC_Inventory::UseInventory()
{
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Green, "Use INVENTORY");
}

void UAC_Inventory::DropInventory()
{
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Black, "Drop INVENTORY");
}

void UAC_Inventory::AddToInventory(FName Name, int32 Quantity)
{
	FS_ItemData* ItemData = ItemDataTable->FindRow<FS_ItemData>(Name, TEXT("Find row in datatable"));

	if (ItemData && ItemData->bCanStack)
	{
		FS_Item* ItemToAdd = Items.FindByPredicate([&](FS_Item& ArrayItem)
			{
				return ArrayItem.Name == Name;
			});

		if (ItemToAdd)
		{
			ItemToAdd->Quantity += Quantity;
		}
		else
		{
			Items.Add(FS_Item(Name, 1));
		}
	}
	else
	{
		Items.Add(FS_Item(Name, 1));
	}
}

void UAC_Inventory::RemoveFromInventory(FName Name, int32 Quantity)
{
	int32 IndexToRemove = Items.IndexOfByPredicate([&](const FS_Item& ArrayItem)
		{
			return ArrayItem.Name == Name;
		});

	if (IndexToRemove != INDEX_NONE)
	{
		if (Items[IndexToRemove].Quantity > Quantity)
		{
			Items[IndexToRemove].Quantity -= Quantity;
		}
		else
		{
			Items.RemoveAt(IndexToRemove);
		}
	}
}