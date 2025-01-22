// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveItemComponent.h"
#include "InteractivePickerComponent.h"

// Sets default values for this component's properties
UInteractiveItemComponent::UInteractiveItemComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	PrimaryComponentTick.bCanEverTick = true;

	bAutoActivate = true;
}

void UInteractiveItemComponent::CallInteractiveSelected(AActor* Owner)
{
	auto Picker = Cast<UInteractivePickerComponent>(Owner->GetComponentByClass(UInteractivePickerComponent::StaticClass()));
	OnInteractorSelected.Broadcast(Picker);
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

void UInteractiveItemComponent::SetIsInteractiveNow(AActor* WhoInteract, bool Value)
{
	if (!IsValid(this))
	{
		return;
	}

	auto Picker = Cast<UInteractivePickerComponent>(WhoInteract->GetComponentByClass(UInteractivePickerComponent::StaticClass()));

	IsInteractiveNow = Value;
	OnInteractiveNow.Broadcast(WhoInteract, IsInteractiveNow);
}


// Called when the game starts
void UInteractiveItemComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UInteractiveItemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

