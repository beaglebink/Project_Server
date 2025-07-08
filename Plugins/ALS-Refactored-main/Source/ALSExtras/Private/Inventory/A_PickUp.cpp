#include "Inventory/A_PickUp.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractiveItemComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractivePickerComponent.h"
#include "Inventory/AC_Inventory.h"
#include "Inventory/AC_Container.h"
#include "Kismet/GameplayStatics.h"


AA_PickUp::AA_PickUp()
{
	PrimaryActorTick.bCanEverTick = false;

	StaticMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	InteractiveComponent = CreateDefaultSubobject<UInteractiveItemComponent>(TEXT("InteractiveComponent"));
	RootComponent = StaticMeshComp;
}

void AA_PickUp::BeginPlay()
{
	Super::BeginPlay();

	InteractiveComponent->OnInteractionPressKeyEvent.AddDynamic(this, &AA_PickUp::AddToInventory);
	InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to pick up"));
}

void AA_PickUp::AddToInventory(UInteractivePickerComponent* Picker)
{
	UAC_Inventory* Inventory = Cast<UAC_Inventory>(Picker->GetOwner()->GetComponentByClass(UAC_Inventory::StaticClass()));
	Inventory->ContainerComponent->AddToContainer(Name, 1);

	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}

	Destroy();
}
