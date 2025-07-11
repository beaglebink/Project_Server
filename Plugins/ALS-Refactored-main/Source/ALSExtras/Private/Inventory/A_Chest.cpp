#include "Inventory/A_Chest.h"
#include "Components/AudioComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractiveItemComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractivePickerComponent.h"
#include "Inventory/AC_Inventory.h"
#include "Kismet/GameplayStatics.h"

AA_Chest::AA_Chest()
{
}

void AA_Chest::TimelineProgress(float Value)
{
	OpenAngle = FMath::Lerp(0.0f, 135.0f, Value);
}

void AA_Chest::BeginPlay()
{
	Super::BeginPlay();

	InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to open"));
}

void AA_Chest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_Chest::Open(UInteractivePickerComponent* Picker)
{
	Super::Open(Picker);

	InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to close"));

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this, Picker]()
		{
			UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.0f);
			UAC_Inventory* Inventory = Cast< UAC_Inventory>(Picker->GetOwner()->GetComponentByClass(UAC_Inventory::StaticClass()));
			Inventory->OpenInventory(EnumInventoryType::Chest, ContainerComponent);
		}, 1.0f, false);
}

void AA_Chest::Close()
{
	Super::Close();

	InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to open"));
}