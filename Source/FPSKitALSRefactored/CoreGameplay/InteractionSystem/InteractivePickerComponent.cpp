#include "InteractivePickerComponent.h"

UInteractivePickerComponent::UInteractivePickerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

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

void UInteractivePickerComponent::SetCurrentItem(UInteractiveItemComponent* FoundItem)
{
	const auto Owner = this->GetOwner();

	if (!IsValid(FoundItem))
	{
		if (IsValid(CurrentItem) || CurrentIItemIsValid)
		{
			LostComponentNow(Owner, CurrentItem);

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

			FoundComponentNow(Owner, FoundItem);
		}
		else
		{
			if (FoundItem != CurrentItem)
			{
				LostComponentNow(Owner, CurrentItem);

				FoundComponentNow(Owner, FoundItem);

				CurrentItem = FoundItem;
				CurrentIItemIsValid = true;
			}
		}
	}
}

void UInteractivePickerComponent::ResetCurrentItem()
{
	CurrentItem = nullptr;
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

	float NearestDistance = TNumericLimits<float>::Max();
	UInteractiveItemComponent* NearestItem = nullptr;

	auto FindNearestComponent = [&](const AActor* Actor)
		{
			TInlineComponentArray<UInteractiveItemComponent*> InteractiveItems;
			Actor->GetComponents<UInteractiveItemComponent>(InteractiveItems, true);

			for (UInteractiveItemComponent* InteractiveItem : InteractiveItems)
			{
				if (!IsValid(InteractiveItem) || !InteractiveItem->IsActive())
				{
					continue;
				}

				float Distance = FVector::Dist(Location, InteractiveItem->GetOwner()->GetActorLocation());
				if (Distance < NearestDistance)
				{
					NearestDistance = Distance;
					NearestItem = InteractiveItem;
				}
			}
		};

	for (const auto& OverlapResult : OutOverlapResults)
	{
		const auto HitActor = OverlapResult.GetActor();
		if (!IsValid(HitActor) || HitActor == GetOwner())
		{
			continue;
		}

		FindNearestComponent(HitActor);
	}

	return NearestItem;
}


void UInteractivePickerComponent::TickSetCurrentItem(UInteractiveItemComponent* FoundItem)
{
	const auto Owner = GetOwner();
	const auto World = GetWorld();

	if (!IsValid(World))
	{
		return;
	}
	SetCurrentItem(FoundItem);
}

void UInteractivePickerComponent::LostComponentNow(AActor* Owner, UInteractiveItemComponent* InteractiveComponent)
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

	if (InteractiveComponent != nullptr && IsValid(InteractiveComponent) && Parent != nullptr && IsValid(Parent))
	{
		InteractiveComponent->FinishInteractiveUse(Parent, false);
	}
}

void UInteractivePickerComponent::FoundComponentNow(AActor* Owner, UInteractiveItemComponent* InteractiveComponent)
{
	if (IsValid(InteractiveComponent))
	{
		InteractiveComponent->SetIsInteractiveNow(Owner);
	}

	OnInteractiveFocusEvent.Broadcast(InteractiveComponent);

}

UInteractiveItemComponent* UInteractivePickerComponent::DoInteractiveUse()
{
	if (IsValid(CurrentItem))
	{
		auto PickerOwner = Cast<ACharacter>(GetOwner());
		CurrentItem->DoInteractiveUse(PickerOwner);
	}

	return CurrentItem;
}
