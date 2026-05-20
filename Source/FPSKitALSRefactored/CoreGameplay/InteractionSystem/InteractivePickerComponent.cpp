#include "InteractivePickerComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "InteractiveActorInterface.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "InteractCommandPayload.h"
#include "Components/ChildActorComponent.h"
#include "WireAndConnections/A_Wire.h"

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

	if (!FoundItem || !CurrentItem)
	{
		LostComponentNow(Owner, CurrentItem);
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
		DrawDebugBox(World, BoxCenter, BoxExtent, BoxQuaternion, FColor::Red, false, 0.5f);
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
	UInteractiveItemComponent* SelectedItem = nullptr;

	TInlineComponentArray<UInteractiveItemComponent*> InteractiveItems;
	TInlineComponentArray<UInteractiveItemComponent*> AllInteractiveItems;

	for (const auto& OverlapResult : OutOverlapResults)
	{
		const auto HitActor = OverlapResult.GetActor();
		if (!HitActor || HitActor == GetOwner())
		{
			continue;
		}

		HitActor->GetComponents<UInteractiveItemComponent>(InteractiveItems, /*bIncludeFromChildActors=*/true);
		AllInteractiveItems.Append(InteractiveItems);
	}

	TInlineComponentArray<UInteractiveItemComponent*> SelectedInteractiveItems;

	for (UInteractiveItemComponent* InteractiveItem : AllInteractiveItems)
	{
		if (!InteractiveItem->IsActive())
		{
			continue;
		}

		float SquareDistance = FVector::DistSquared(Location, InteractiveItem->GetOwner()->GetActorLocation());
		float Radius = InteractiveItem->InteractionRange;

		if (SquareDistance > Radius * Radius)
		{
			continue;
		}

		if (InteractiveItem && DebugDraw)
		{
			UE_LOG(LogTemp, Warning, TEXT("Item: %s %f"), *InteractiveItem->GetOwner()->GetName(), SquareDistance);
		}

		SelectedInteractiveItems.Add(InteractiveItem);

		if (SquareDistance < NearestDistance)
		{
			NearestDistance = SquareDistance;
			NearestItem = InteractiveItem;
		}
	}

	// Determine crosshair world ray using player controller (use screen center if no custom crosshair)
	FVector2D CrosshairPosition = FVector2D::ZeroVector;
	FVector CrosshairDirection = Direction;
	FVector CrosshairWorldOrigin = Location;

	APlayerController* PlayerController = nullptr;
	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		PlayerController = Cast<APlayerController>(OwnerPawn->GetController());
	}
	if (!PlayerController)
	{
		PlayerController = World->GetFirstPlayerController();
	}

	if (PlayerController)
	{
		int32 ViewX = 0, ViewY = 0;
		PlayerController->GetViewportSize(ViewX, ViewY);
		if (ViewX > 0 && ViewY > 0)
		{
			if (UFunction* Func = PlayerController->FindFunction(FName(TEXT("GetCrosshairPosition"))))
			{
				struct FGetCrosshairPosition_Params
				{
					FVector2D ReturnValue;
				} Params;

				PlayerController->ProcessEvent(Func, &Params);
				CrosshairPosition = Params.ReturnValue;
			}
			else
			{
				CrosshairPosition = FVector2D(ViewX * 0.5f, ViewY * 0.5f);
			}

			FVector DeprojOrigin;
			FVector DeprojDir;
			if (PlayerController->DeprojectScreenPositionToWorld(CrosshairPosition.X, CrosshairPosition.Y, DeprojOrigin, DeprojDir))
			{
				CrosshairDirection = DeprojDir.GetSafeNormal();
				CrosshairWorldOrigin = DeprojOrigin;
			}
		}
	}

	TArray<FHitResult> OutHits;
	const FVector LineStart = Location;
	const FVector LineEnd = CrosshairWorldOrigin + CrosshairDirection * Depth;

	bool bLineTraceHit = false;

	// Ћайнтрейс по лучу прицела, берем компонент на ближайшем экторе
	bLineTraceHit = World->LineTraceMultiByChannel(OutHits, LineStart, LineEnd, ECollisionChannel::ECC_Camera);
	if (bLineTraceHit)
	{
		for (auto& Hit : OutHits)
		{
			if (!Hit.GetActor()) continue;
			AActor* HitActor = Hit.GetActor();

			UInteractiveItemComponent* ItemFromHit = HitActor ? HitActor->FindComponentByClass<UInteractiveItemComponent>() : nullptr;
			if (ItemFromHit)
			{
				if (ItemFromHit->IsActive())
				{
					SelectedItem = ItemFromHit;
					break;
				}
			}
		}
	}
	FVector DebugSphereLocation{ FVector::ZeroVector };
	if (SelectedItem)
	{
		float InteractionSquareDistance = FVector::DistSquared(Location, SelectedItem->GetOwner()->GetActorLocation());
		float InteractionRadius = SelectedItem->InteractionRange;
		DebugSphereLocation = SelectedItem->GetOwner()->GetActorLocation();

		//Wire - check what connnector is closer to the linetrace
		if (AA_Wire* Wire = Cast<AA_Wire>(SelectedItem->GetOwner()))
		{
			Wire->bIsMaleUsed = true;
			if (FVector::DistSquared(Location, Wire->GetActorLocation()) > FVector::DistSquared(Location, Wire->FemaleMeshComponent->GetComponentLocation()))
			{
				InteractionSquareDistance = FVector::DistSquared(Location, Wire->FemaleMeshComponent->GetComponentLocation());
				DebugSphereLocation = Wire->FemaleMeshComponent->GetComponentLocation();
				Wire->bIsMaleUsed = false;
			}
		}

		if (DebugDraw)
		{
			DrawDebugSphere(World, DebugSphereLocation, InteractionRadius, 32, InteractionSquareDistance > FMath::Square(InteractionRadius) ? FColor::Cyan : FColor::Yellow, false, 0.5f);
		}

		if (InteractionSquareDistance > InteractionRadius * InteractionRadius)
		{
			SelectedItem = nullptr;
		}
	}

	if (SelectedItem && DebugDraw)
	{
		UE_LOG(LogTemp, Warning, TEXT("NearestItem: %s %f"), *SelectedItem->GetOwner()->GetName(), NearestDistance);

		DrawDebugLine(World, LineStart + Direction * Depth * 0.03f, SelectedItem->GetOwner()->GetActorLocation(), FColor::Blue, false, 1.f, 0, .5f);
		DrawDebugSphere(World, DebugSphereLocation, 6.f, 8, FColor::Yellow, false, .5f);
	}

	return SelectedItem;
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
	OnInteractiveLostFocusEvent.Broadcast();

	if (!Owner || (!InteractiveComponent && !CurrentIItemIsValid))
	{
		return;
	}

	const auto Parent = Cast<ACharacter>(GetOwner());

	if (!Parent)
	{
		return;
	}

	if (InteractiveComponent)
	{
		InteractiveComponent->OnInteractStateChanged.RemoveAll(this);
		InteractiveComponent->OnInteractTooltipChange.RemoveAll(this);

		// GetOwner() вернЄт ChildActor (если компонент на нЄм) или обычный актор Ч
		// в обоих случа€х EnableHighlight вызываетс€ точно на нужном акторе
		AActor* ItemActor = InteractiveComponent->GetOwner();
		if (ItemActor && ItemActor->GetClass()->ImplementsInterface(UInteractiveActorInterface::StaticClass()))
		{
			IInteractiveActorInterface::Execute_EnableHighlight(ItemActor, false);
		}
	}

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
		InteractiveComponent->OnInteractTooltipChange.AddDynamic(this, &UInteractivePickerComponent::HandleInteractTooltipChange);

		// GetOwner() вернЄт ChildActor (если компонент на нЄм) или обычный актор Ч
		// в обоих случа€х EnableHighlight вызываетс€ точно на нужном акторе
		AActor* ItemActor = InteractiveComponent->GetOwner();
		if (ItemActor && ItemActor->GetClass()->ImplementsInterface(UInteractiveActorInterface::StaticClass()))
		{
			IInteractiveActorInterface::Execute_EnableHighlight(ItemActor, true);
		}
	}

	OnInteractiveReceiveFocusEvent.Broadcast(InteractiveComponent);
}

// ¬ методе DoInteractiveUse замен€ем пр€мой вызов компонента на публикацию команды в EventBus
UInteractiveItemComponent* UInteractivePickerComponent::DoInteractiveUse()
{
	if (!CurrentItem)
	{
		return nullptr;
	}

	if (UEventBusSubsystem* EventBus = GetWorld()->GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		if (UInteractCommandPayload* P = EventBus->CreatePayload<UInteractCommandPayload>())
		{
			P->ItemId = CurrentItem->GetItemId();
			P->Picker = this;

			FOutcomeEventBase Outcome;
			Outcome.Payload = P;

			switch (CurrentItem->SubsystemType)
			{
			case EInteractiveSubsystem::Interior:
				Outcome.OutcomeType = EOutcomeType::Interior;
				break;
			case EInteractiveSubsystem::ActorNPC:
				Outcome.OutcomeType = EOutcomeType::Actor;
				break;
			case EInteractiveSubsystem::Inventory:
				Outcome.OutcomeType = EOutcomeType::Inventory;
				break;
			case EInteractiveSubsystem::Terminal:
				Outcome.OutcomeType = EOutcomeType::Terminal;
				break;
			default:
				Outcome.OutcomeType = EOutcomeType::Default;
				break;
			}

			EventBus->PublishOutcome(Outcome);
		}
	}

	return CurrentItem;
}

void UInteractivePickerComponent::HandleInteractTooltipChange(const FText& NewTooltip)
{
	OnInteractTooltipChange.Broadcast(NewTooltip);
}
