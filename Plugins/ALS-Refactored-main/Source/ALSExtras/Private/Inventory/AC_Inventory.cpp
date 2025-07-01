#include "Inventory/AC_Inventory.h"
#include "Inventory/A_PickUp.h"
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
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()->GetInstigatorController()))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		}
	}

	InputComponent->BindAction(InventoryAction, ETriggerEvent::Triggered, this, &UAC_Inventory::ToggleInventory);
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

void UAC_Inventory::OpenInventory()
{
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Green, "OPEN INVENTORY");

	bIsOpen = true;
	Subsystem->AddMappingContext(Inventory_IMContext, 1);
}

void UAC_Inventory::CloseInventory()
{
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Red, "CLOSE INVENTORY");

	bIsOpen = false;
	Subsystem->RemoveMappingContext(Inventory_IMContext);
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

	if (ItemDataTable->FindRow<FS_ItemData>(Name, TEXT("Find row in datatable"))->bCanStack)
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
	FS_Item* ItemToRemove = Items.FindByPredicate([&](FS_Item& ArrayItem)
		{
			return ArrayItem.Name == Name;
		});

	if (ItemToRemove)
	{
		if (ItemToRemove->Quantity > Quantity)
		{
			ItemToRemove->Quantity -= Quantity;
		}
		else
		{
			Items.Remove(*ItemToRemove);
		}
	}
}