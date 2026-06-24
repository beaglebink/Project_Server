#include "FloorAssignmentComponent.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "FloorPlacementPayload.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UFloorAssignmentComponent::UFloorAssignmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFloorAssignmentComponent::BeginPlay()
{
	Super::BeginPlay();

}

void UFloorAssignmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SnapshotChannel == ESnapshotChannel::None || !ItemId.IsValid())
		return;

	if (EndPlayReason != EEndPlayReason::Destroyed)
		return;

	// Publish unregistration via EventBus
	if (UWorld* W = GetWorld())
	{
		if (UGameInstance* GI = W->GetGameInstance())
		{
			if (UEventBusSubsystem* Bus = GI->GetSubsystem<UEventBusSubsystem>())
			{
				UFloorPlacementPayload* Payload = Bus->CreatePayload<UFloorPlacementPayload>();
				if (Payload)
				{
					Payload->Setup(ActorType, GetOwner()->GetActorTransform(), ItemId/*, GetOwner()*/);

					FOutcomeEventBase Ev;
					Ev.OutcomeType = EOutcomeType::Interior;
					Ev.OutcomeInterior = EOutcomeInterior::FloorPlacementUnregistered;
					Ev.Payload = Payload;

					Bus->PublishOutcome(Ev);
				}
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UFloorAssignmentComponent::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);
	/*
	// При дублировании в редакторе (Ctrl+D) генерируем новый ItemId для дубликата
	if (DuplicateMode == EDuplicateMode::Normal)
	{
		ItemId = FGuid::NewGuid();
		// Можно также сбросить SnapshotChannel, если нужно:
		// SnapshotChannel = ESnapshotChannel::None;
	}
	*/
}
