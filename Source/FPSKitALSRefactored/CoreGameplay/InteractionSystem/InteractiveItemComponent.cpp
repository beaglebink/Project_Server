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

	OnInteractiveLostFocusEvent.Broadcast(ReleasedUser);
}

void UInteractiveItemComponent::SetIsInteractiveNow(AActor* WhoInteract)
{
	if (!IsValid(this))
	{
		return;
	}

	auto Picker = Cast<UInteractivePickerComponent>(WhoInteract->GetComponentByClass(UInteractivePickerComponent::StaticClass()));

	OnInteractiveReceiveFocusEvent.Broadcast(WhoInteract);
}

void UInteractiveItemComponent::DoInteractiveUse(ACharacter* IIUser)
{
	if (!IsValid(this))
	{
		return;
	}

	if (IsActive())
	{
		auto Picker = Cast<UInteractivePickerComponent>(IIUser->GetComponentByClass(UInteractivePickerComponent::StaticClass()));

		OnInteractionPressKeyEvent.Broadcast(Picker);
	}
}
