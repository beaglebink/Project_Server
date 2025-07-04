#include "Inventory/A_Chest.h"
#include "Components/SkeletalMeshComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractiveItemComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractivePickerComponent.h"
#include "Inventory/AC_Container.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Inventory/AC_Inventory.h"


AA_Chest::AA_Chest()
{
	PrimaryActorTick.bCanEverTick = true;

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SKM_Chest"));
	InteractiveComponent = CreateDefaultSubobject<UInteractiveItemComponent>(TEXT("InteractiveComponent"));
	ContainerComponent = CreateDefaultSubobject<UAC_Container>(TEXT("ContainerComponent"));
	OpenTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("OpenTimeline"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));

	RootComponent = SkeletalMeshComponent;

	AudioComponent->bAutoActivate = false;
	AudioComponent->SetupAttachment(RootComponent);
}

void AA_Chest::TimelineProgress(float Value)
{
	OpenAngle = FMath::Lerp(0.0f, 135.0f, Value);

}

void AA_Chest::TimelineFinished()
{
	bInProcess = false;

	if (!bIsOpen)
	{
		bIsOpen = true;
		if (CloseSound)
		{
			AudioComponent->Stop();
			AudioComponent->SetSound(CloseSound);
		}
	}
	else
	{
		bIsOpen = false;
		if (OpenSound)
		{
			AudioComponent->Stop();
			AudioComponent->SetSound(OpenSound);
		}
	}
}

void AA_Chest::BeginPlay()
{
	Super::BeginPlay();

	if (FloatCurve)
	{
		ProgressFunction.BindUFunction(this, FName("TimelineProgress"));
		OpenTimeline->AddInterpFloat(FloatCurve, ProgressFunction);

		FinishedFunction.BindUFunction(this, FName("TimelineFinished"));
		OpenTimeline->SetTimelineFinishedFunc(FinishedFunction);

		OpenTimeline->SetLooping(false);
	}

	InteractiveComponent->OnInteractionPressKeyEvent.AddDynamic(this, &AA_Chest::OpenCloseChest);
	InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to open"));
	InteractiveComponent->OnInteractiveLostFocusEvent.AddDynamic(this, &AA_Chest::OnLostFocus);

	if (OpenSound)
	{
		AudioComponent->SetSound(OpenSound);
	}
}

void AA_Chest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_Chest::OpenCloseChest(UInteractivePickerComponent* Picker)
{
	OpenChest(Picker);
	CloseChest();
}

void AA_Chest::OpenChest(UInteractivePickerComponent* Picker)
{
	if (bInProcess || bIsOpen)
	{
		return;
	}

	bInProcess = true;

	AudioComponent->Play();
	OpenTimeline->PlayFromStart();
	InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to close"));

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this, Picker]()
		{
			UAC_Inventory* Inventory = Cast< UAC_Inventory>(Picker->GetOwner()->GetComponentByClass(UAC_Inventory::StaticClass()));
			Inventory->OpenInventory(EnumInventoryType::Chest, ContainerComponent);
		}, 1.0f, false);
}

void AA_Chest::CloseChest()
{
	if (bInProcess || !bIsOpen)
	{
		return;
	}

	bInProcess = true;

	AudioComponent->Play();
	OpenTimeline->ReverseFromEnd();
	InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to open"));
}

void AA_Chest::OnCloseInventoryEvent_Implementation()
{
	CloseChest();
}

void AA_Chest::OnLostFocus(ACharacter* Character)
{
	UInteractivePickerComponent* Picker = Cast< UInteractivePickerComponent>(Character->GetComponentByClass(UInteractivePickerComponent::StaticClass()));
	if (Picker)
	{
		CloseChest();
	}
}

