#include "InteractivePickerComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UInteractivePickerComponent::UInteractivePickerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInteractivePickerComponent::BeginPlay()
{
	Super::BeginPlay();

	const auto OwnerPawn = Cast<APawn>(GetOwner());
	check(OwnerPawn && TEXT("Designed for player pawns only! Better crash than silence!"));
	if (OwnerPawn)
	{
		TimerDel = FTimerDelegate::CreateUObject(this, &UInteractivePickerComponent::TickPicker, 0.f);
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, PickTickInterval, true);
	}
}

void UInteractivePickerComponent::SetCurrentItem(UInteractiveItemComponent* FoundItem)
{
	const auto Owner = GetOwner();

	if (!FoundItem)
	{
		if (CurrentItem || CurrentIItemIsValid)
		{
			LostComponentNow(Owner, CurrentItem);

			CurrentItem = nullptr;
			CurrentIItemIsValid = false;
		}
	}
	else
	{
		if (!CurrentItem && !CurrentIItemIsValid)
		{
			CurrentItem = FoundItem;
			CurrentIItemIsValid = true;

			FoundComponentNow(Owner, FoundItem);
		}
		else if (FoundItem != CurrentItem)
		{
			LostComponentNow(Owner, CurrentItem);

			FoundComponentNow(Owner, FoundItem);

			CurrentItem = FoundItem;
			CurrentIItemIsValid = true;
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

	if (!Character || !Character->IsLocallyControlled())
	{
		return;
	}

	const APlayerController* CurrentController = Cast<APlayerController>(Character->GetController());
	if (!CurrentController)
	{
		return;
	}

	const USkeletalMeshComponent* CharacterMesh = Character->GetMesh();
	if (!CharacterMesh)
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

		FoundItem = TraceNearestUsableObject(Location, Direction, TracedActors, ActorsToIgnore);

		TickSetCurrentItem(FoundItem);
	}
}

UInteractiveItemComponent* UInteractivePickerComponent::TraceNearestUsableObject(const FVector& Location, const FVector& Direction, TMap<AActor*, FString>& OutTracedActors, const TArray<AActor*>& ActorsToIgnore) const
{
	const auto World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const auto BoxExtent = FVector(Depth / 2.f, Width, Width);
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
	const auto TraceResult = World->OverlapMultiByChannel(OutOverlapResults, BoxCenter, BoxQuaternion, ECollisionChannel::ECC_Camera, CollisionShape);
	if (!TraceResult)
	{
		return nullptr;
	}

	float NearestDistance = TNumericLimits<float>::Max();
	UInteractiveItemComponent* NearestItem = nullptr;

	TInlineComponentArray<UInteractiveItemComponent*> InteractiveItems;
	TInlineComponentArray<UInteractiveItemComponent*> AllInteractiveItems;

	for (const auto& OverlapResult : OutOverlapResults)
	{
		const auto HitActor = OverlapResult.GetActor();
		if (!HitActor || HitActor == GetOwner())
		{
			continue;
		}

		HitActor->GetComponents<UInteractiveItemComponent>(InteractiveItems, true);
		AllInteractiveItems.Append(InteractiveItems);
	}

	for (UInteractiveItemComponent* InteractiveItem : AllInteractiveItems)
	{
		if (!InteractiveItem->IsActive())
		{
			continue;
		}

		float Distance = FVector::DistSquared(Location, InteractiveItem->GetOwner()->GetActorLocation());
		if (InteractiveItem && DebugDraw)
		{
			UE_LOG(LogTemp, Warning, TEXT("Item: %s %f"), *InteractiveItem->GetOwner()->GetName(), Distance);
		}

		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			NearestItem = InteractiveItem;
		}
	}

	if (NearestItem && DebugDraw)
	{
		UE_LOG(LogTemp, Warning, TEXT("NearestItem: %s %f"), *NearestItem->GetOwner()->GetName(), NearestDistance);
	}
;
	return NearestItem;
}

void UInteractivePickerComponent::TickSetCurrentItem(UInteractiveItemComponent* FoundItem)
{
	const auto Owner = GetOwner();
	const auto World = GetWorld();

	if (!World)
	{
		return;
	}
	SetCurrentItem(FoundItem);
}

void UInteractivePickerComponent::LostComponentNow(AActor* Owner, UInteractiveItemComponent* InteractiveComponent)
{
	if (!Owner || (!InteractiveComponent && !CurrentIItemIsValid))
	{
		return;
	}

	const auto Parent = Cast<ACharacter>(GetOwner());

	if (!Parent)
	{
		return;
	}

	OnInteractiveLostFocusEvent.Broadcast();

	if (InteractiveComponent && Parent)
	{
		InteractiveComponent->FinishInteractiveUse(Parent, false);
	}
}

void UInteractivePickerComponent::FoundComponentNow(AActor* Owner, UInteractiveItemComponent* InteractiveComponent)
{
	if (InteractiveComponent)
	{
		InteractiveComponent->SetIsInteractiveNow(Owner);
	}

	OnInteractiveReceiveFocusEvent.Broadcast(InteractiveComponent);
}

UInteractiveItemComponent* UInteractivePickerComponent::DoInteractiveUse()
{
	if (CurrentItem)
	{
		auto PickerOwner = Cast<ACharacter>(GetOwner());
		CurrentItem->DoInteractiveUse(PickerOwner);

		OnInteractionPressKeyEvent.Broadcast();
	}

	return CurrentItem;
}