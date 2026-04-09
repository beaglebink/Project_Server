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

	// Ensure ItemId exists for stable identification.
	// Editor: generate deterministic GUID based on actor path so it's stable between editor sessions/saves.
	// Runtime (packaged/play): fallback to NewGuid() for transient instances.
	if (!ItemId.IsValid())
	{
#if WITH_EDITOR
		// Deterministic GUID derived from actor path Ч stable and reproducible in editor
		const FString ObjectPath = GetOwner() ? GetOwner()->GetPathName() : FString();
		ItemId = FGuid::NewDeterministicGuid(ObjectPath, 0);

		// Persist generated id into the level instance so it's stable in editor sessions.
		if (!GetOwner()->HasAnyFlags(RF_ClassDefaultObject))
		{
			Modify();
			GetOwner()->Modify();
			if (UPackage* Pkg = GetOwner()->GetOutermost())
			{
				Pkg->MarkPackageDirty();
			}
		}
#else
		// Runtime: create transient GUID
		ItemId = FGuid::NewGuid();
#endif
	}

	// Publish placement registration via EventBus
	if (UWorld* W = GetWorld())
	{
		if (UGameInstance* GI = W->GetGameInstance())
		{
			if (UEventBusSubsystem* Bus = GI->GetSubsystem<UEventBusSubsystem>())
			{
				UFloorPlacementPayload* Payload = Bus->CreatePayload<UFloorPlacementPayload>();
				if (Payload)
				{
					// ѕередаЄм полный трансформ владельца
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