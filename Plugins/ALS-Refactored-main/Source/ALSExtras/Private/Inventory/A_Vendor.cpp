#include "Inventory/A_Vendor.h"
#include "Components/AudioComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractiveItemComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractivePickerComponent.h"
#include "Inventory/AC_Inventory.h"
#include "Kismet/GameplayStatics.h"

AA_Vendor::AA_Vendor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AA_Vendor::TimelineProgress(float Value)
{
}

void AA_Vendor::BeginPlay()
{
	Super::BeginPlay();

	InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to trade"));
}

void AA_Vendor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_Vendor::Open(UInteractivePickerComponent* Picker)
{
	Super::Open(Picker);

	InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to leave"));

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this, Picker]()
		{
			UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.0f);
			UAC_Inventory* Inventory = Cast< UAC_Inventory>(Picker->GetOwner()->GetComponentByClass(UAC_Inventory::StaticClass()));
			Inventory->OpenInventory(EnumInventoryType::Vendor, ContainerComponent);
		}, 1.2f, false);
}

void AA_Vendor::Close()
{
	Super::Close();

	InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to trade"));
}

