#include "InteractiveItemComponent.h"
#include "InteractivePickerComponent.h"

UInteractiveItemComponent::UInteractiveItemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	PrimaryComponentTick.bCanEverTick = true;

	bAutoActivate = true;
}
void UInteractiveItemComponent::FinishInteractiveUse(ACharacter* IIUser, const bool IsReleaseButton)
{
	if (this == nullptr || !IsValid(this))
	{
		return;
	}

	if (IIUser == nullptr || !IsValid(IIUser))
	{
		return;
	}

	ReleasedUser = IIUser;
	IsRelease = IsReleaseButton;

	OnInteractiveFinishUseEvent.Broadcast(ReleasedUser);

	if (IsRelease)
	{
		OnUseReleaseKeyEvent.Broadcast(ReleasedUser);
	}
}

void UInteractiveItemComponent::SetIsInteractiveNow(AActor* WhoInteract)
{
	if (!IsValid(this))
	{
		return;
	}

	auto Picker = Cast<UInteractivePickerComponent>(WhoInteract->GetComponentByClass(UInteractivePickerComponent::StaticClass()));

	OnInteractiveNow.Broadcast(WhoInteract);
}


void UInteractiveItemComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

