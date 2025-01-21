// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractivePickerComponent.h"
//#include "ALSCamera/Public/AlsCameraComponent.h"

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

	//auto Camera = Character->GetComponentByClass(UAlsCameraComponent::StaticClass());
	FMinimalViewInfo MinimalViewInfo;
	Character->CalcCamera(0.f, MinimalViewInfo);

	if (IsActive())
	{
		const FVector Location = MinimalViewInfo.Location;
		const FRotator Rotation = MinimalViewInfo.Rotation;
		const FVector Direction = Rotation.Vector();

		TArray<AActor*> ActorsToIgnore(ActorsToIgnoreCache);

		//ActorsToIgnore.Add(Character);
		ActorsToIgnore = FoundCharacters;


#if (!UE_BUILD_SHIPPING)
		if (IsShowInteractionTrace)
		{
			TracedActors.Empty();
		}
#endif //!UE_BUILD_SHIPPING

		FoundItem = TraceNearestUsableObject(Location, Direction, TracedActors, GetPickRadius(), ActorsToIgnore);
	}
}

UInteractiveItemComponent* UInteractivePickerComponent::TraceNearestUsableObject(const FVector& Location, const FVector& Direction, TMap<AActor*, FString>& OutTracedActors, float Radius, const TArray<AActor*>& ActorsToIgnore) const
{
	const auto World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	const auto BoxExtent = FVector(Radius / 2.f, CapsuleRadius, CapsuleRadius);
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
/*
	UClass* OwnerClass = GetOwner()->GetClass();

	auto OwnerTypeFilter = [this, OwnerClass](FOverlapResult Result)
	{
		const AActor* OverlappedActor = Result.GetActor();
		if (OverlappedActor != nullptr)
		{
			const AActor* Owner = this->GetOwner();
			if (Owner != nullptr)
			{
				if (OverlappedActor->IsA(OwnerClass))
				{
					return OverlappedActor != Owner;
				}
			}
		}
		return false;
	};

	auto CharacterData = OutOverlapResults.FindByPredicate(OwnerTypeFilter);
*/
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
#if (!UE_BUILD_SHIPPING)
				if (IsShowInteractionTrace)
				{
					OutTracedActors.Add(HitActor, FString(TEXT("Interactive item deactivated")));
				}
#endif //!UE_BUILD_SHIPPING
				continue;
			}

#if (!UE_BUILD_SHIPPING)
			if (IsShowInteractionTrace)
			{
				OutTracedActors.Add(HitActor, FString(TEXT("Interactive USED")));
			}
#endif //!UE_BUILD_SHIPPING
			return InteractiveItem;
		}
#if (!UE_BUILD_SHIPPING)
		if (IsShowInteractionTrace)
		{
			if (!OutTracedActors.Contains(HitActor))
			{
				OutTracedActors.Add(HitActor, FString(TEXT("Block interact")));
			}
		}
#endif //!UE_BUILD_SHIPPING
	}

	return nullptr;
}
