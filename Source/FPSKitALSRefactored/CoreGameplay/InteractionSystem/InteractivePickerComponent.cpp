// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractivePickerComponent.h"

// Sets default values for this component's properties
UInteractivePickerComponent::UInteractivePickerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UInteractivePickerComponent::BeginPlay()
{
	Super::BeginPlay();

	const auto OwnerPawn = Cast<APawn>(GetOwner());
	check(OwnerPawn && TEXT("Designed for player pawns only! Better crash than silence!"));
	if (IsValid(OwnerPawn))
	{
		TimerDel = FTimerDelegate::CreateUObject(this, &UInteractivePickerComponent::TickPicker, 0.f);
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, PickTickInterval, true);
	}
	
}


// Called every frame
void UInteractivePickerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UInteractivePickerComponent::SetCurrentItem(UInteractiveItemComponent* FoundItem, bool IsPeriodicalUpdate)
{
	const auto Owner = this->GetOwner();

	if (!IsValid(FoundItem))
	{
		if (IsValid(CurrentItem) || CurrentIItemIsValid)
		{
			LostComponentNow(Owner, CurrentItem, IsPeriodicalUpdate);

			CurrentItem = nullptr;
			CurrentIItemIsValid = false;
		}
	}
	else
	{
		if (!IsValid(CurrentItem) && CurrentIItemIsValid == false)
		{
			CurrentItem = FoundItem;
			CurrentIItemIsValid = true;

			FoundComponentNow(Owner, FoundItem, IsPeriodicalUpdate);
		}
		else
		{
			if (FoundItem != CurrentItem)
			{
				LostComponentNow(Owner, CurrentItem, IsPeriodicalUpdate);

				FoundComponentNow(Owner, FoundItem, IsPeriodicalUpdate);

				OnInteractiveSelected.Broadcast(FoundItem);

				FoundItem->CallInteractiveSelected(Owner);

				CurrentItem = FoundItem;
				CurrentIItemIsValid = true;
			}
		}
	}
}

void UInteractivePickerComponent::OnStartUsePressKeyEvent(ACharacter* Character)
{
	OnPickerStartUsePressKeyEvent.Broadcast();

	OnInteractionStarted.Broadcast(CurrentItem);

}

void UInteractivePickerComponent::OnUseReleaseKeyEvent(AActor* Initiator)
{
	OnPickerEndHoldUseEvent.Broadcast();
}

	void UInteractivePickerComponent::TickPicker(float DeltaTime)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	check(Character && TEXT("Designed for player pawns only!"));

	if (!IsValid(Character))
	{
		return;
	}

	if (!Character->IsLocallyControlled())
	{
		return;
	}

	const APlayerController* CurrentController = Cast<APlayerController>(Character->GetController());
	if (!IsValid(CurrentController))
	{
		return;
	}

	const USkeletalMeshComponent* CharacterMesh = Character->GetMesh();
	if (CharacterMesh == nullptr)
	{
		return;
	}

	TArray<UInteractiveItemComponent*> ItemsNearby;
	UInteractiveItemComponent* FoundItem = nullptr;

	FMinimalViewInfo MinimalViewInfo;
	Character->CalcCamera(0.f, MinimalViewInfo);

	if (IsActive())
	{
		const FVector Location = MinimalViewInfo.Location;
		const FRotator Rotation = MinimalViewInfo.Rotation;
		const FVector Direction = Rotation.Vector();

		TArray<AActor*> ActorsToIgnore(ActorsToIgnoreCache);

		ActorsToIgnore = FoundCharacters;

		FoundItem = TraceNearestUsableObject(Location, Direction, TracedActors, GetPickRadius(), ActorsToIgnore);

		TickSetCurrentItem(FoundItem);
	}
}

UInteractiveItemComponent* UInteractivePickerComponent::TraceNearestUsableObject(const FVector& Location, const FVector& Direction, TMap<AActor*, FString>& OutTracedActors, float Radius, const TArray<AActor*>& ActorsToIgnore) const
{
	const auto World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	const auto BoxExtent = FVector(Radius / 2.f, Width, Width);
	const auto BoxCenter = Location + Direction * BoxExtent.X;
	const auto BoxQuaternion = Direction.ToOrientationQuat();
	if (DebugDraw)
	{
		DrawDebugBox(World, BoxCenter, BoxExtent, BoxQuaternion, FColor::Red, false, 4.f);
	}

	FCollisionShape CollisionShape;
	CollisionShape.ShapeType = ECollisionShape::Box;
	CollisionShape.SetBox(FVector3f(BoxExtent));
	TArray<FOverlapResult> OutOverlapResults;
	const auto TraceResult = GetWorld()->OverlapMultiByChannel(OutOverlapResults, BoxCenter, BoxQuaternion, ECollisionChannel::ECC_Camera, CollisionShape);
	if (!TraceResult)
	{
		return nullptr;
	}

	for (const auto OverlapResult : OutOverlapResults)
	{
		const auto HitActor = OverlapResult.GetActor();
		if (!IsValid(HitActor))
		{
			continue;
		}

		if (HitActor == GetOwner())
		{
			continue;
		}

		TArray<UInteractiveItemComponent*> InteractiveItems;
		HitActor->GetComponents<UInteractiveItemComponent>(InteractiveItems, /*bIncludeFromChildActors =*/false);

		for (UInteractiveItemComponent* InteractiveItem : InteractiveItems)
		{
			if (!IsValid(InteractiveItem))
			{
				continue;
			}

			return InteractiveItem;
		}
	}

	return nullptr;
}

void UInteractivePickerComponent::TickSetCurrentItem(UInteractiveItemComponent* FoundItem)
{
	const auto Owner = GetOwner();
	const auto World = GetWorld();

	if (!IsValid(World))
	{
		return;
	}

	if (CurrentItem != nullptr && IsValid(CurrentItem) && !CurrentItem->IsActive())
	{
		SetCurrentItem(FoundItem, true);
		return;
	}
}

void UInteractivePickerComponent::LostComponentNow(AActor* Owner, UInteractiveItemComponent* InteractiveComponent, bool IsPeriodicalUpdate)
{
	if (!IsValid(Owner))
	{
		return;
	}

	if (!IsValid(InteractiveComponent) && !CurrentIItemIsValid)
	{
		return;
	}

	const auto Parent = Cast<ACharacter>(GetOwner());

	if (!IsValid(Parent))
	{
		return;
	}

	OnInteractiveLostFocusEvent.Broadcast();
	OnInteractiveRemoved.Broadcast(InteractiveComponent);

	if (InteractiveComponent != nullptr && IsValid(InteractiveComponent) && Parent != nullptr && IsValid(Parent))
	{
		InteractiveComponent->FinishInteractiveUse(Parent, false);
	}

	if (IsValid(InteractiveComponent))
	{

		InteractiveComponent->OnStartUsePressKeyEvent.RemoveDynamic(this, &UInteractivePickerComponent::OnStartUsePressKeyEvent);
		InteractiveComponent->OnUseReleaseKeyEvent.RemoveDynamic(this, &UInteractivePickerComponent::OnUseReleaseKeyEvent);

	}
}

void UInteractivePickerComponent::FoundComponentNow(AActor* Owner, UInteractiveItemComponent* InteractiveComponent, bool IsPeriodicalUpdate)
{
	if (IsValid(InteractiveComponent))
	{
		InteractiveComponent->SetIsInteractiveNow(Owner, true);
	}

	OnInteractiveFocusEvent.Broadcast(InteractiveComponent);

	if (IsValid(InteractiveComponent))
	{
		InteractiveComponent->OnStartUsePressKeyEvent.AddUniqueDynamic(this, &UInteractivePickerComponent::OnStartUsePressKeyEvent);
		InteractiveComponent->OnUseReleaseKeyEvent.AddUniqueDynamic(this, &UInteractivePickerComponent::OnUseReleaseKeyEvent);
	}
}
