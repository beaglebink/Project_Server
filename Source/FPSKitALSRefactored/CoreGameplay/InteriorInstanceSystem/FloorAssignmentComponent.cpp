#include "FloorAssignmentComponent.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "FloorPlacementPayload.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UFloorAssignmentComponent::UFloorAssignmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	//ItemId = FGuid::NewGuid();
}

void UFloorAssignmentComponent::BeginPlay()
{
	Super::BeginPlay();

    // If ItemId is absent — generate a runtime GUID.
    // (Если ItemId отсутствует — генерируем runtime GUID.)
    // In editor the ItemId is set by the tool and saved to .umap, so we don't modify assets here.
    // (В редакторе ItemId ставится инструментом и сохраняется в .umap, поэтому здесь не модифицируем пакеты.)
	/*
	if (!ItemId.IsValid())
	{
		ItemId = FGuid::NewGuid();
	}
	*/

    // Publish placement registration via EventBus
    // (Публикуем регистрацию размещения через EventBus)
	if (UWorld* W = GetWorld())
	{
		if (UGameInstance* GI = W->GetGameInstance())
		{
			if (UEventBusSubsystem* Bus = GI->GetSubsystem<UEventBusSubsystem>())
			{
				UFloorPlacementPayload* Payload = Bus->CreatePayload<UFloorPlacementPayload>();
				if (Payload)
				{
					Payload->Setup(InteriorSetId, FloorId, ActorType, AnchorId, GetOwner()->GetActorTransform(), ItemId, GetOwner());

					FOutcomeEventBase Ev;
					Ev.OutcomeType = EOutcomeType::Interior;
					Ev.OutcomeInterior = EOutcomeInterior::FloorPlacementRegistered;
					Ev.Payload = Payload;

					Bus->PublishOutcome(Ev);
				}
			}
		}
	}
}

void UFloorAssignmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
					Payload->Setup(InteriorSetId, FloorId, ActorType, AnchorId, GetOwner()->GetActorTransform(), ItemId, GetOwner());

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