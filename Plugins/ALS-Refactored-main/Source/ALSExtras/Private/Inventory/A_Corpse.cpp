#include "Inventory/A_Corpse.h"
#include "Components/AudioComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractiveItemComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractivePickerComponent.h"
#include "Inventory/AC_Inventory.h"

AA_Corpse::AA_Corpse()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AA_Corpse::TimelineProgress(float Value)
{
	RotateAngle = FMath::Lerp(0.0f, 180.0f, Value);
}

void AA_Corpse::BeginPlay()
{
	Super::BeginPlay();

	InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to search"));
}

void AA_Corpse::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_Corpse::Open(UInteractivePickerComponent* Picker)
{
	Super::Open(Picker);

	InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to leave"));


	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this, Picker]()
		{
			UAC_Inventory* Inventory = Cast< UAC_Inventory>(Picker->GetOwner()->GetComponentByClass(UAC_Inventory::StaticClass()));
			Inventory->OpenInventory(EnumInventoryType::Corpse, ContainerComponent);
		}, 1.2f, false);
}

void AA_Corpse::Close()
{
	Super::Close();

	InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to search"));
}

