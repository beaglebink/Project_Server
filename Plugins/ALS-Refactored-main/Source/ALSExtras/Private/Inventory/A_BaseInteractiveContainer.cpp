#include "Inventory/A_BaseInteractiveContainer.h"
#include "Components/AudioComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractiveItemComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractivePickerComponent.h"
#include "Inventory/AC_Container.h"

AA_BaseInteractiveContainer::AA_BaseInteractiveContainer()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SKM_Chest"));
	InteractiveComponent = CreateDefaultSubobject<UInteractiveItemComponent>(TEXT("InteractiveComponent"));
	ContainerComponent = CreateDefaultSubobject<UAC_Container>(TEXT("ContainerComponent"));
	OpenTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("OpenTimeline"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));

	RootComponent = SceneComponent;

	SkeletalMeshComponent->SetupAttachment(SceneComponent);
	AudioComponent->SetupAttachment(RootComponent);

	AudioComponent->bAutoActivate = false;
}

void AA_BaseInteractiveContainer::TimelineProgress(float Value)
{
}

void AA_BaseInteractiveContainer::TimelineFinished()
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

void AA_BaseInteractiveContainer::BeginPlay()
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

	InteractiveComponent->OnInteractionPressKeyEvent.AddDynamic(this, &AA_BaseInteractiveContainer::OpenClose);
	InteractiveComponent->OnInteractiveLostFocusEvent.AddDynamic(this, &AA_BaseInteractiveContainer::OnLostFocus);

	if (OpenSound)
	{
		AudioComponent->SetSound(OpenSound);
	}
}

void AA_BaseInteractiveContainer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_BaseInteractiveContainer::OpenClose(UInteractivePickerComponent* Picker)
{
	Open(Picker);
	Close();
}

void AA_BaseInteractiveContainer::Open(UInteractivePickerComponent* Picker)
{
	if (bInProcess || bIsOpen)
	{
		return;
	}
	bInProcess = true;
	AudioComponent->Play();
	OpenTimeline->PlayFromStart();
}

void AA_BaseInteractiveContainer::Close()
{
	if (bInProcess || !bIsOpen)
	{
		return;
	}
	bInProcess = true;
	AudioComponent->Play();
	OpenTimeline->ReverseFromEnd();
}

void AA_BaseInteractiveContainer::OnCloseInventoryEvent_Implementation()
{
	Close();
}

void AA_BaseInteractiveContainer::OnLostFocus(ACharacter* Character)
{
	UInteractivePickerComponent* Picker = Cast< UInteractivePickerComponent>(Character->GetComponentByClass(UInteractivePickerComponent::StaticClass()));
	if (Picker)
	{
		Close();
	}
}

