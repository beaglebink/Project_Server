#include "InteriorSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../EventBusSystem/OutcomeConditionAsset.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractItemStatePayload.h"
#include "../InteractionSystem/InteractiveItemComponent.h"
#include "../InteractionSystem/InteractCommandPayload.h"
#include "../InteractionSystem/InteractSetEnabledPayload.h"
#include "../InteractionSystem/InteractSetRangePayload.h"
#include "../InteractionSystem/InteractSetTooltipPayload.h"
#include "FloorStatePayload.h"
#include "../MissionSystem/ReleaseMissionSnapshotPayload.h"
#include "../MissionSystem/MissionEnvelopePayload.h"
#include "../WorldStateSystem/WorldStateRecordPayload.h"
#include "FloorAssignmentComponent.h"
#include "UObject/UnrealType.h"
#include "EngineUtils.h"
#include "Components/ChildActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "../LocationSystem/InteriorSetAsset.h"
#include "../LocationSystem/FloorAsset.h"
#include "../LocationSystem/LocationSpatialTypes.h"
#include "InteriorTransitionPayload.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "PlayerController_I.h"
#include "CoreBlueprintFunctionLibrary.h"
#include "../LocationSystem/LocationAnchorActor.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UpdateMissionListPayload.h"

// -----------------------------------------------------------------------------
// Snapshot helpers
// -----------------------------------------------------------------------------
namespace
{
	static void CollectSaveGameProperties(UObject* Obj, TArray<FFloorSavedPropertyEntry>& OutProps)
	{
		if (!IsValid(Obj)) return;
		for (TFieldIterator<FProperty> It(Obj->GetClass()); It; ++It)
		{
			FProperty* Prop = *It;
			if (!Prop->HasAllPropertyFlags(CPF_SaveGame)) continue;

			FString Exported;
			Prop->ExportText_InContainer(0, Exported, Obj, nullptr, Obj, PPF_None);
			OutProps.Add({ Prop->GetFName(), MoveTemp(Exported) });
		}
	}

	static void ApplySaveGameProperties(UObject* Obj, const TArray<FFloorSavedPropertyEntry>& Props)
	{
		if (!IsValid(Obj)) return;
		for (const FFloorSavedPropertyEntry& Entry : Props)
		{
			FProperty* Prop = FindFProperty<FProperty>(Obj->GetClass(), Entry.PropertyName);
			if (!Prop) continue;
			Prop->ImportText_InContainer(*Entry.ValueText, Obj, Obj, PPF_None);
		}
	}

	static void RestoreSceneComponentTransform(USceneComponent* Comp, const FFloorSavedComponentState& CState)
	{
		if (!IsValid(Comp)) return;

		if (CState.bWasAttached)
		{
			AActor* Owner = Comp->GetOwner();
			if (IsValid(Owner))
			{
				for (UActorComponent* AC : Owner->GetComponents())
				{
					USceneComponent* ParentSC = Cast<USceneComponent>(AC);
					if (ParentSC && ParentSC->GetFName() == CState.AttachParentName)
					{
						Comp->AttachToComponent(ParentSC, FAttachmentTransformRules::KeepRelativeTransform, CState.AttachSocketName);
						break;
					}
				}
			}
		}

		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Comp))
		{
			const bool bWasSim = CState.bWasSimulatingPhysics;
			if (bWasSim)
			{
				Prim->SetSimulatePhysics(false);

				if (CState.bHasRelativeTransform)
					Comp->SetRelativeTransform(CState.RelativeTransform, false, nullptr, ETeleportType::TeleportPhysics);
				else if (CState.bHasWorldTransform && !CState.bWasAttached)
					Comp->SetWorldTransform(CState.WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);

				Prim->SetSimulatePhysics(true);
				Prim->SetPhysicsLinearVelocity(CState.SavedLinearVelocity);
				Prim->SetPhysicsAngularVelocityInDegrees(CState.SavedAngularVelocityDeg);
			}
			else
			{
				if (CState.bHasRelativeTransform)
					Comp->SetRelativeTransform(CState.RelativeTransform, false, nullptr, ETeleportType::None);
				else if (CState.bHasWorldTransform && !CState.bWasAttached)
					Comp->SetWorldTransform(CState.WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
		else
		{
			if (CState.bHasRelativeTransform)
				Comp->SetRelativeTransform(CState.RelativeTransform, false, nullptr, ETeleportType::None);
			else if (CState.bHasWorldTransform && !CState.bWasAttached)
				Comp->SetWorldTransform(CState.WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}

	static void SnapshotActor(AActor* Actor, FFloorSavedActorState& OutSnapshot)
	{
		if (!IsValid(Actor)) return;

		UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>();
		if (!Comp) return;

		OutSnapshot.ActorTransform = Actor->GetActorTransform();
		CollectSaveGameProperties(Actor, OutSnapshot.ActorProperties);

		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);
		for (UActorComponent* C : Components)
		{
			if (!IsValid(C)) continue;
			TArray<FFloorSavedPropertyEntry> Props;
			CollectSaveGameProperties(C, Props);

			FFloorSavedComponentState CState;
			CState.ComponentName      = C->GetFName();
			CState.ComponentClassName = C->GetClass()->GetFName();
			CState.bWasActive         = C->IsActive();
			CState.Properties         = MoveTemp(Props);

			if (USceneComponent* SC = Cast<USceneComponent>(C))
			{
				if (USceneComponent* ParentSC = SC->GetAttachParent())
				{
					CState.bWasAttached      = true;
					CState.AttachParentName  = ParentSC->GetFName();
					CState.AttachSocketName  = SC->GetAttachSocketName();
					CState.RelativeTransform = SC->GetRelativeTransform();
					CState.bHasRelativeTransform = true;
				}
				CState.WorldTransform    = SC->GetComponentTransform();
				CState.bHasWorldTransform = true;
			}

			if (UPrimitiveComponent* PC = Cast<UPrimitiveComponent>(C))
			{
				CState.bWasSimulatingPhysics = PC->IsSimulatingPhysics();
				if (CState.bWasSimulatingPhysics)
				{
					CState.SavedLinearVelocity     = PC->GetPhysicsLinearVelocity();
					CState.SavedAngularVelocityDeg = PC->GetPhysicsAngularVelocityInDegrees();
				}
			}

			OutSnapshot.ComponentStates.Add(MoveTemp(CState));
		}
	}

	static void RestoreActorSnapshot(AActor* Actor, const FFloorSavedActorState& Snapshot, bool bApplyWorldTransform = true)
	{
		if (!IsValid(Actor)) return;

		if (bApplyWorldTransform)
			Actor->SetActorTransform(Snapshot.ActorTransform, false, nullptr, ETeleportType::TeleportPhysics);

		ApplySaveGameProperties(Actor, Snapshot.ActorProperties);

		TMap<FName, UActorComponent*> ComponentsByName;
		for (UActorComponent* C : Actor->GetComponents())
		{
			if (!IsValid(C)) continue;
			ComponentsByName.Add(C->GetFName(), C);
		}

		for (const FFloorSavedComponentState& CState : Snapshot.ComponentStates)
		{
			if (!CState.bWasAttached) continue;
			UActorComponent** Found = ComponentsByName.Find(CState.ComponentName);
			if (!Found || !IsValid(*Found)) continue;
			USceneComponent* TargetSC = Cast<USceneComponent>(*Found);
			if (!TargetSC) continue;
			UActorComponent** ParentFound = ComponentsByName.Find(CState.AttachParentName);
			if (ParentFound && IsValid(*ParentFound))
			{
				if (USceneComponent* ParentSC = Cast<USceneComponent>(*ParentFound))
					TargetSC->AttachToComponent(ParentSC, FAttachmentTransformRules::KeepRelativeTransform, CState.AttachSocketName);
			}
		}

		for (const FFloorSavedComponentState& CState : Snapshot.ComponentStates)
		{
			UActorComponent** Found = ComponentsByName.Find(CState.ComponentName);
			if (!Found || !IsValid(*Found)) continue;
			UActorComponent* Target = *Found;

			ApplySaveGameProperties(Target, CState.Properties);

			if (Target->IsActive() != CState.bWasActive)
				CState.bWasActive ? Target->Activate(true) : Target->Deactivate();

			if (USceneComponent* SC = Cast<USceneComponent>(Target))
				RestoreSceneComponentTransform(SC, CState);
		}
	}
} // namespace

// -----------------------------------------------------------------------------
// SaveFloorActorsState
// -----------------------------------------------------------------------------

void UInteriorSubsystem::SaveFloorActorsState(const FGuid& InteriorSetId, const FGuid& FloorId, FName MissionId, EJobSpacePolicy JobSpacePolicy, const TArray<FEnvelopeChannelEntry> &Channels)
{
    UWorld* W = GetWorld();
    if (!W) return;

    FInteriorFloorKey Key(InteriorSetId, FloorId);

    auto ShouldSkipForPartial = [&](EFloorActorType ActorType) -> bool
    {
        if (JobSpacePolicy != EJobSpacePolicy::Partial) return false;
        const EEnvelopeChannel EnvelopeChannel = FloorActorTypeToEnvelopeChannel(ActorType);
        for (const FEnvelopeChannelEntry& Channel : Channels)
        {
            if (Channel.Channel == EnvelopeChannel && Channel.Policy == EChannelPolicy::Reset)
                return true;
        }
        return false;
    };

    if (MissionId == NAME_None)
    {
        // записать baseline в FloorStateSnapshots (перезаписать)
        TArray<FFloorSavedActorState>& Bucket = FloorStateSnapshots.FindOrAdd(Key);
        Bucket.Empty();

        for (TActorIterator<AActor> It(W); It; ++It)
        {
            AActor* Actor = *It;
            if (!IsValid(Actor)) continue;
            UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>();
            if (!Comp) continue;
            if (Comp->SnapshotChannel == ESnapshotChannel::None) continue;

            // Пропускаем актёра, если политика Partial и для его типа канал помечен как Reset
            if (ShouldSkipForPartial(Comp->ActorType))
                continue;

            FFloorSavedActorState Snapshot;
            Snapshot.ItemId = Comp->ItemId;
            SnapshotActor(Actor, Snapshot);
            Bucket.Add(MoveTemp(Snapshot));

            // child actors (как в оригинале)
            TArray<UChildActorComponent*> ChildComps;
            Actor->GetComponents<UChildActorComponent>(ChildComps);
            for (UChildActorComponent* ChildComp : ChildComps)
            {
                if (!IsValid(ChildComp)) continue;
                AActor* ChildActor = ChildComp->GetChildActor();
                if (!IsValid(ChildActor)) continue;
                UFloorAssignmentComponent* ChildFloorComp = ChildActor->FindComponentByClass<UFloorAssignmentComponent>();
                if (!ChildFloorComp) continue;
                if (ChildFloorComp->SnapshotChannel == ESnapshotChannel::None) continue;
                if (ChildFloorComp->GetInteriorSetId() != InteriorSetId) continue;

                // Пропускаем дочерний актёр по той же логике
                if (ShouldSkipForPartial(ChildFloorComp->ActorType))
                    continue;

                FFloorSavedActorState ChildSnapshot;
                ChildSnapshot.ItemId = ChildFloorComp->ItemId;
                ChildSnapshot.RelativeTransform = ChildComp->GetRelativeTransform();
                ChildSnapshot.bHasRelativeTransform = true;
                SnapshotActor(ChildActor, ChildSnapshot);
                Bucket.Add(MoveTemp(ChildSnapshot));
            }
        }

        UE_LOG(LogTemp, Log,
            TEXT("InteriorSubsystem: SaveFloorActorsState — saved baseline for floor %s/%s (total: %d)"),
            *InteriorSetId.ToString(), *FloorId.ToString(), Bucket.Num());
        return;
    }

    // существующая логика: сохраняем в MissionFloorSnapshots[Key][MissionId]
    TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = MissionFloorSnapshots.FindOrAdd(Key);
    TArray<FFloorSavedActorState>& Bucket = PerFloor.FindOrAdd(MissionId);

    TMap<FGuid, int32> ExistingIndex;
    for (int32 i = 0; i < Bucket.Num(); ++i)
        ExistingIndex.Add(Bucket[i].ItemId, i);

    int32 AddedCount   = 0;
    int32 UpdatedCount = 0;

    for (TActorIterator<AActor> It(W); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor)) continue;

        UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>();
        if (!Comp) continue;
        if (Comp->SnapshotChannel == ESnapshotChannel::None) continue;

        // Пропускаем актёра при Partial+Reset
        if (ShouldSkipForPartial(Comp->ActorType))
            continue;

        FFloorSavedActorState Snapshot;
        Snapshot.ItemId = Comp->ItemId;
        SnapshotActor(Actor, Snapshot);

        if (int32* FoundIdx = ExistingIndex.Find(Snapshot.ItemId))
        {
            Bucket[*FoundIdx] = MoveTemp(Snapshot);
            ++UpdatedCount;
        }
        else
        {
            Bucket.Add(MoveTemp(Snapshot));
            ExistingIndex.Add(Bucket.Last().ItemId, Bucket.Num() - 1);
            ++AddedCount;
        }

        TArray<UChildActorComponent*> ChildComps;
        Actor->GetComponents<UChildActorComponent>(ChildComps);
        for (UChildActorComponent* ChildComp : ChildComps)
        {
            if (!IsValid(ChildComp)) continue;
            AActor* ChildActor = ChildComp->GetChildActor();
            if (!IsValid(ChildActor)) continue;

            UFloorAssignmentComponent* ChildFloorComp = ChildActor->FindComponentByClass<UFloorAssignmentComponent>();
            if (!ChildFloorComp) continue;
            if (ChildFloorComp->SnapshotChannel == ESnapshotChannel::None) continue;
            if (ChildFloorComp->GetInteriorSetId() != InteriorSetId) continue;

            // Пропускаем дочерний актёр при Partial+Reset
            if (ShouldSkipForPartial(ChildFloorComp->ActorType))
                continue;

            FFloorSavedActorState ChildSnapshot;
            ChildSnapshot.ItemId              = ChildFloorComp->ItemId;
            ChildSnapshot.RelativeTransform   = ChildComp->GetRelativeTransform();
            ChildSnapshot.bHasRelativeTransform = true;
            SnapshotActor(ChildActor, ChildSnapshot);

            if (int32* ChildIdx = ExistingIndex.Find(ChildSnapshot.ItemId))
            {
                Bucket[*ChildIdx] = MoveTemp(ChildSnapshot);
                ++UpdatedCount;
            }
            else
            {
                Bucket.Add(MoveTemp(ChildSnapshot));
                ExistingIndex.Add(Bucket.Last().ItemId, Bucket.Num() - 1);
                ++AddedCount;
            }
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("InteriorSubsystem: SaveFloorActorsState — added %d, updated %d for mission '%s' floor %s/%s (total: %d)"),
        AddedCount, UpdatedCount, *MissionId.ToString(),
        *InteriorSetId.ToString(), *FloorId.ToString(), Bucket.Num());
}

// -----------------------------------------------------------------------------
// RestoreFloorActorsState
// -----------------------------------------------------------------------------

int32 UInteriorSubsystem::RestoreFloorActorsState(const FGuid& InteriorSetId, const FGuid& FloorId)
{
    UWorld* W = GetWorld();
    if (!W) return 0;

    FInteriorFloorKey Key(InteriorSetId, FloorId);

    // Ищем все снимки миссий для данного этажа
    const TMap<FName, TArray<FFloorSavedActorState>>* PerFloor = MissionFloorSnapshots.Find(Key);
    if (!PerFloor || PerFloor->Num() == 0) return 0;

    // Собираем актёров этажа в словарь по ItemId
    TMap<FGuid, AActor*> ActorByItemId;
    for (TActorIterator<AActor> It(W); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor)) continue;
        if (UFloorAssignmentComponent* C = Actor->FindComponentByClass<UFloorAssignmentComponent>())
        {
            if (C->SnapshotChannel != ESnapshotChannel::None)
                ActorByItemId.Add(C->ItemId, Actor);
        }

        TArray<UChildActorComponent*> ChildComps;
        Actor->GetComponents<UChildActorComponent>(ChildComps);
        for (UChildActorComponent* ChildComp : ChildComps)
        {
            if (!IsValid(ChildComp)) continue;
            AActor* ChildActor = ChildComp->GetChildActor();
            if (!IsValid(ChildActor)) continue;
            if (UFloorAssignmentComponent* CC = ChildActor->FindComponentByClass<UFloorAssignmentComponent>())
            {
                if (CC->SnapshotChannel != ESnapshotChannel::None)
                    ActorByItemId.Add(CC->ItemId, ChildActor);
            }
        }
    }

    int32 TotalRestored = 0;

    // Восстанавливаем снимки для каждой миссии, привязанной к этому этажу
    for (const auto& MissionPair : *PerFloor)
    {
        const TArray<FFloorSavedActorState>& Snapshots = MissionPair.Value;

        // First pass: независимые (root) акторы
        for (const FFloorSavedActorState& Snapshot : Snapshots)
        {
            if (Snapshot.bHasRelativeTransform) continue;
            AActor** ActorPtr = ActorByItemId.Find(Snapshot.ItemId);
            if (!ActorPtr || !IsValid(*ActorPtr)) continue;
            RestoreActorSnapshot(*ActorPtr, Snapshot, true);
            ++TotalRestored;
        }

        // Second pass: дочерние (child) акторы
        for (const FFloorSavedActorState& Snapshot : Snapshots)
        {
            if (!Snapshot.bHasRelativeTransform) continue;
            AActor** ActorPtr = ActorByItemId.Find(Snapshot.ItemId);
            if (!ActorPtr || !IsValid(*ActorPtr)) continue;
            AActor* ChildActor = *ActorPtr;

            AActor* ParentActor = ChildActor->GetAttachParentActor();
            bool bAppliedRelative = false;
            if (IsValid(ParentActor))
            {
                TArray<UChildActorComponent*> ParentChildComps;
                ParentActor->GetComponents<UChildActorComponent>(ParentChildComps);
                for (UChildActorComponent* ParentChildComp : ParentChildComps)
                {
                    if (!IsValid(ParentChildComp)) continue;
                    if (ParentChildComp->GetChildActor() == ChildActor)
                    {
                        ParentChildComp->SetRelativeTransform(Snapshot.RelativeTransform);
                        bAppliedRelative = true;
                        break;
                    }
                }
            }

            if (!bAppliedRelative)
                ChildActor->SetActorTransform(Snapshot.ActorTransform, false, nullptr, ETeleportType::TeleportPhysics);

            RestoreActorSnapshot(ChildActor, Snapshot, false);
            ++TotalRestored;
        }

        UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: RestoreFloorActorsState — restored %d actors for mission '%s' floor %s/%s"),
            TotalRestored, *MissionPair.Key.ToString(), *InteriorSetId.ToString(), *FloorId.ToString());
    }

    return TotalRestored;
}

// --- добавлен новый хелпер: восстанавливает actors напрямую из переданного массива снимков ---
static int32 RestoreFromSnapshotArray(UWorld* W, const TArray<FFloorSavedActorState>& Snapshots, const FGuid& InteriorSetId, const FGuid& FloorId)
{
	if (!W) return 0;
	if (Snapshots.IsEmpty()) return 0;

	TMap<FGuid, AActor*> ActorByItemId;
	for (TActorIterator<AActor> It(W); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor)) continue;
		if (UFloorAssignmentComponent* C = Actor->FindComponentByClass<UFloorAssignmentComponent>())
		{
			if (C->SnapshotChannel != ESnapshotChannel::None)
			{
				ActorByItemId.Add(C->ItemId, Actor);
			}
		}

		TArray<UChildActorComponent*> ChildComps;
		Actor->GetComponents<UChildActorComponent>(ChildComps);
		for (UChildActorComponent* ChildComp : ChildComps)
		{
			if (!IsValid(ChildComp)) continue;
			AActor* ChildActor = ChildComp->GetChildActor();
			if (!IsValid(ChildActor)) continue;
			if (UFloorAssignmentComponent* CC = ChildActor->FindComponentByClass<UFloorAssignmentComponent>())
			{
				if (CC->SnapshotChannel != ESnapshotChannel::None)
				{
					ActorByItemId.Add(CC->ItemId, ChildActor);
				}
			}
		}
	}

	int32 Restored = 0;

	// First pass: non-relative (root) actors
	for (const FFloorSavedActorState& Snapshot : Snapshots)
	{
		if (Snapshot.bHasRelativeTransform) continue;
		AActor** ActorPtr = ActorByItemId.Find(Snapshot.ItemId);
		if (!ActorPtr || !IsValid(*ActorPtr)) continue;
		RestoreActorSnapshot(*ActorPtr, Snapshot, true);
		++Restored;
	}

	// Second pass: relative (child) actors
	for (const FFloorSavedActorState& Snapshot : Snapshots)
	{
		if (!Snapshot.bHasRelativeTransform) continue;
		AActor** ActorPtr = ActorByItemId.Find(Snapshot.ItemId);
		if (!ActorPtr || !IsValid(*ActorPtr)) continue;
		AActor* ChildActor = *ActorPtr;

		AActor* ParentActor = ChildActor->GetAttachParentActor();
		bool bAppliedRelative = false;
		if (IsValid(ParentActor))
		{
			TArray<UChildActorComponent*> ParentChildComps;
			ParentActor->GetComponents<UChildActorComponent>(ParentChildComps);
			for (UChildActorComponent* ParentChildComp : ParentChildComps)
			{
				if (!IsValid(ParentChildComp)) continue;
				if (ParentChildComp->GetChildActor() == ChildActor)
				{
					ParentChildComp->SetRelativeTransform(Snapshot.RelativeTransform);
					bAppliedRelative = true;
					break;
				}
			}
		}

		if (!bAppliedRelative)
			ChildActor->SetActorTransform(Snapshot.ActorTransform, false, nullptr, ETeleportType::TeleportPhysics);

		RestoreActorSnapshot(ChildActor, Snapshot, false);
		++Restored;
	}

	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: RestoreFromSnapshotArray — restored %d actors for floor %s/%s"),
		Restored, *InteriorSetId.ToString(), *FloorId.ToString());

	return Restored;
}

// -----------------------------------------------------------------------------
// EventBus handlers — Registration
// -----------------------------------------------------------------------------

void UInteriorSubsystem::HandleInteractRegistration(const FOutcomeEventBase& Outcome)
{
	UInteractItemRegistrationPayload* P = Cast<UInteractItemRegistrationPayload>(Outcome.Payload);
	if (!P) return;

	AActor* Owner = P->GetOwnerActor();
	const FString OwnerName = Owner ? Owner->GetName() : FString(TEXT("Unknown"));

	if (Outcome.OutcomeInterior == EOutcomeInterior::InteractRegistered)
	{
		FInteractItemRecord R;
		R.ItemId           = P->ItemId;
		R.OwnerActor       = Owner;
		R.InteractionRange = P->InteractionRange;
		R.SubsystemType    = P->SubsystemType;
		RegisteredItems.Add(P->ItemId, R);

		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Registered item '%s' (range=%.1f)"), *OwnerName, P->InteractionRange);

		if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* List = RegistrationListeners.Find(P->ItemId))
		{
			for (auto It = List->CreateIterator(); It; ++It)
			{
				if (UInteractiveItemComponent* Comp = It->Get())
					Comp->OnRegisteredBySubsystem(P);
			}
			RegistrationListeners.Remove(P->ItemId);
		}

		OnInteractItemRegistered.Broadcast(P);
	}
	else if (Outcome.OutcomeInterior == EOutcomeInterior::InteractUnregistered)
	{
		RegisteredItems.Remove(P->ItemId);

		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Unregistered item '%s'"), *OwnerName);

		if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* List = RegistrationListeners.Find(P->ItemId))
		{
			for (auto It = List->CreateIterator(); It; ++It)
			{
				if (UInteractiveItemComponent* Comp = It->Get())
					Comp->OnUnregisteredBySubsystem(P);
			}
			RegistrationListeners.Remove(P->ItemId);
		}

		OnInteractItemUnregistered.Broadcast(P);
	}
}

void UInteriorSubsystem::HandlePlacementRegistration(const FOutcomeEventBase& Outcome)
{
	UFloorPlacementPayload* P = Cast<UFloorPlacementPayload>(Outcome.Payload);
	if (!P) return;

	FInteriorFloorKey Key(P->InteriorSetId, P->FloorId);

	if (Outcome.OutcomeInterior == EOutcomeInterior::FloorPlacementRegistered)
	{
		FFloorPopulationRecord NewRecord;
		NewRecord.ActorType      = P->ActorType;
		NewRecord.AnchorId       = P->AnchorId;
		NewRecord.WorldTransform = P->WorldTransform;
		NewRecord.PlacedActor    = P->OwnerActor.Get();
		NewRecord.bHasAnchor     = P->AnchorId.IsValid();

		FFloorPopulationBuckets& Buckets = SpawnedActorsByInteriorFloor.FindOrAdd(Key);

		switch (P->ActorType)
		{
		case EFloorActorType::HeavyFurniture: Buckets.HeavyFurniture.Add(MoveTemp(NewRecord)); break;
		case EFloorActorType::LightItem:      Buckets.LightItems.Add(MoveTemp(NewRecord));      break;
		case EFloorActorType::Terminal:       Buckets.Terminals.Add(MoveTemp(NewRecord));       break;
		case EFloorActorType::NPC_Spawner:    Buckets.NPCSpawners.Add(MoveTemp(NewRecord));     break;
		case EFloorActorType::Debris:         Buckets.Debris.Add(MoveTemp(NewRecord));          break;
		default:                              Buckets.LightItems.Add(MoveTemp(NewRecord));      break;
		}

		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Placement registered for floor %s/%s (AnchorId=%s)"),
			*P->InteriorSetId.ToString(), *P->FloorId.ToString(), *P->AnchorId.ToString());
	}
	else if (Outcome.OutcomeInterior == EOutcomeInterior::FloorPlacementUnregistered)
	{
		if (FFloorPopulationBuckets* Buckets = SpawnedActorsByInteriorFloor.Find(Key))
		{
			const FGuid AnchorToRemove = P->AnchorId;
			auto RemoveByAnchor = [&AnchorToRemove](TArray<FFloorPopulationRecord>& Arr)
			{
				Arr.RemoveAll([&AnchorToRemove](const FFloorPopulationRecord& R) { return R.AnchorId == AnchorToRemove; });
			};
			RemoveByAnchor(Buckets->HeavyFurniture);
			RemoveByAnchor(Buckets->LightItems);
			RemoveByAnchor(Buckets->Terminals);
			RemoveByAnchor(Buckets->NPCSpawners);
			RemoveByAnchor(Buckets->Debris);
		}

		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Placement unregistered for floor %s/%s (AnchorId=%s)"),
			*P->InteriorSetId.ToString(), *P->FloorId.ToString(), *P->AnchorId.ToString());
	}
}

// -----------------------------------------------------------------------------
// EventBus handlers — Interact commands
// -----------------------------------------------------------------------------

void UInteriorSubsystem::HandleInteractCommand(const FOutcomeEventBase& Outcome)
{
	const UInteractCommandPayload* P = Cast<UInteractCommandPayload>(Outcome.Payload);
	if (!P) return;
	if (const FInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
	{
		if (AActor* Owner = Rec->OwnerActor.Get())
			ExecuteInteractCommandOnOwner(P->ItemId, Owner, P->Picker);
	}
}

void UInteriorSubsystem::HandleSetEnabled(const FOutcomeEventBase& Outcome)
{
	const UInteractSetEnabledPayload* P = Cast<UInteractSetEnabledPayload>(Outcome.Payload);
	if (!P) return;
	if (const FInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
		ExecuteSetEnabledOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->bEnabled);
}

void UInteriorSubsystem::HandleSetRange(const FOutcomeEventBase& Outcome)
{
	const UInteractSetRangePayload* P = Cast<UInteractSetRangePayload>(Outcome.Payload);
	if (!P) return;
	if (const FInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
		ExecuteSetRangeOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->NewRange);
}

void UInteriorSubsystem::HandleSetTooltip(const FOutcomeEventBase& Outcome)
{
	const UInteractSetTooltipPayload* P = Cast<UInteractSetTooltipPayload>(Outcome.Payload);
	if (!P) return;
	if (const FInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
		ExecuteSetTooltipOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->NewTooltip);
}

// -----------------------------------------------------------------------------
// EventBus handlers — Floor snapshot
// -----------------------------------------------------------------------------

void UInteriorSubsystem::HandleFloorStateSave(const FOutcomeEventBase& Outcome)
{
	if (UFloorStatePayload* P = Cast<UFloorStatePayload>(Outcome.Payload))
		SaveFloorActorsState(P->InteriorSetId, P->FloorId, P->MissionId, P->Policy, P->Channels);
}

void UInteriorSubsystem::HandleFloorStateRestore(const FOutcomeEventBase& Outcome)
{
	if (UFloorStatePayload* P = Cast<UFloorStatePayload>(Outcome.Payload))
	{
		if (P->MissionId != NAME_None)
		{
			RestoreMissionFloorState(P->MissionId, FInteriorFloorKey(P->InteriorSetId, P->FloorId));
		}
		else
		{
			RestoreFloorActorsState(P->InteriorSetId, P->FloorId);
		}
	}
}

// -----------------------------------------------------------------------------
// HandleFloorTransition
// -----------------------------------------------------------------------------

void UInteriorSubsystem::HandleFloorTransition(const FOutcomeEventBase& Outcome)
{
	UInteriorTransitionPayload* P = Cast<UInteriorTransitionPayload>(Outcome.Payload);
    if (!P || !P->IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("InteriorSubsystem::HandleFloorTransition: payload invalid (payload невалиден)"));
        OnTransitionCompleted.Broadcast(false, FLocationAnchorLink(), false);
        return;
    }

	TransitionPayloadCache = P;
	UWorld* W = GetWorld();
	if (!W)
	{
		OnTransitionCompleted.Broadcast(false, FLocationAnchorLink(), false);
		return;
	}

	// Получаем путь целевого уровня из payload
	const FString TargetLevelPath = P->GetTargetLevelPackageName();
    if (TargetLevelPath.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("InteriorSubsystem::HandleFloorTransition: failed to determine target level (не удалось определить целевой уровень)"));
        OnTransitionCompleted.Broadcast(false, FLocationAnchorLink(), false);
        return;
    }

	// --- ВАЖНО: объявляем TargetAnchorID здесь, чтобы он был виден далее в функции ---
	const FGuid TargetAnchorID = P->GetTargetAnchorID();

	// Получаем короткое имя текущего уровня через GameplayStatics — параметр true убирает PIE‑префиксы
	FString CurrentLevelShort = UGameplayStatics::GetCurrentLevelName(W, true);
	CurrentLevelShort = CurrentLevelShort.ToLower();

	// Функция нормализации имени из package → base filename
	auto NormalizeNameFromPackage = [](const FString& InPath) -> FString
	{
		if (InPath.IsEmpty()) return FString();

		FString PackagePath = InPath;
		if (PackagePath.Contains(TEXT(".")))
		{
			PackagePath = FPackageName::ObjectPathToPackageName(PackagePath);
		}

		const FString Base = FPaths::GetBaseFilename(PackagePath);

		FString Result = Base;
		int32 PIEPos = Result.Find(TEXT("UEDPIE_"), ESearchCase::IgnoreCase);
		if (PIEPos != INDEX_NONE)
		{
			int32 MPos = Result.Find(TEXT("_M_"), ESearchCase::IgnoreCase, ESearchDir::FromStart, PIEPos);
			if (MPos != INDEX_NONE && MPos + 3 < Result.Len())
				Result = Result.Mid(MPos + 3);
			else if (PIEPos + 6 < Result.Len())
				Result = Result.Mid(PIEPos + 6);
		}

		return Result.ToLower();
	};

	const FString NormTarget = NormalizeNameFromPackage(TargetLevelPath);

	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem::HandleFloorTransition: currentShort='%s', targetPackage='%s' (norm='%s'), anchor=[%s]"),
		*CurrentLevelShort, *TargetLevelPath, *NormTarget, *TargetAnchorID.ToString());

	// Сравниваем короткое имя текущего уровня с нормализованным именем целевого уровня
    if (!CurrentLevelShort.IsEmpty() && !NormTarget.IsEmpty() && CurrentLevelShort == NormTarget)
    {
        UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: same map (by name) — teleporting in-place (та же карта (по имени) — телепортируем на месте)"));
        const bool bOk = TeleportToAnchor(TargetAnchorID);
        OnTransitionCompleted.Broadcast(bOk, TransitionPayloadCache->DestinationLink, false);
        return;
    }

	SetPendingAnchorID(TargetAnchorID);

	APlayerController* PC = W->GetFirstPlayerController();
	if (PC && PC->GetClass()->ImplementsInterface(UPlayerController_I::StaticClass()))
	{
		IPlayerController_I::Execute_SetLoadScreen(PC, true);
		IPlayerController_I::Execute_StoreWeaponState(PC);
	}

	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: SeamlessTravel → '%s'"), *TargetLevelPath);

	FTimerDelegate TravelDelegate = FTimerDelegate::CreateLambda([TargetLevelPath, this]()
	{
		if (UWorld* LocalW = GetWorld())
			LocalW->SeamlessTravel(TargetLevelPath, true);
	});

	// Уведомляем подписчиков: сейчас будет уход с исходного этажа
	// Сначала оповестим через EventBus, чтобы MissionSubsystem мог запросить сохранение mission-snapshots.
	if (UEventBusSubsystem* EventBus = CachedEventBus.Get() ? CachedEventBus.Get() : GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		// Reuse existing payload P (contains SourceFloor info)
		FOutcomeEventBase Ev;
		Ev.OutcomeType = EOutcomeType::Interior;
		Ev.OutcomeInterior = EOutcomeInterior::FloorLeaving; // notify listeners that we're leaving this floor
		Ev.Payload = P;
		EventBus->PublishOutcome(Ev);
	}
	// Затем стандартный локальный broadcast
	OnFloorExiting.Broadcast(P->SourceFloor);

	W->GetTimerManager().SetTimer(TravelTimerHandle, TravelDelegate, 1.0f, false);
}

// -----------------------------------------------------------------------------
// TeleportToAnchor
// -----------------------------------------------------------------------------

bool UInteriorSubsystem::TeleportToAnchor(const FGuid& AnchorID)
{
	if (!AnchorID.IsValid()) return false;

	UWorld* W = GetWorld();
	if (!W) return false;

	ALocationAnchorActor* FoundAnchor = nullptr;
	for (TActorIterator<ALocationAnchorActor> It(W); It; ++It)
	{
		if (It->AnchorID == AnchorID)
		{
			FoundAnchor = *It;
			break;
		}
	}

	if (!FoundAnchor)
	{
		UE_LOG(LogTemp, Warning, TEXT("InteriorSubsystem::TeleportToAnchor: якорь [%s] не найден"), *AnchorID.ToString());
		return false;
	}

	const FVector TargetLocation  = FoundAnchor->GetActorLocation();
	const FRotator TargetRotation = FoundAnchor->GetActorRotation();

	APlayerController* PC = W->GetFirstPlayerController();
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("InteriorSubsystem::TeleportToAnchor: нет PlayerController"));
		return false;
	}

	if (APawn* Pawn = PC->GetPawn())
	{
		Pawn->TeleportTo(TargetLocation, TargetRotation);
		PC->SetControlRotation(TargetRotation);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem::TeleportToAnchor: телепорт в '%s' pos=%s rot=%s"),
			*FoundAnchor->DisplayName.ToString(), *TargetLocation.ToString(), *TargetRotation.ToString());
	}
	else
	{
		SetPendingSpawnTransform(TargetLocation, TargetRotation);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem::TeleportToAnchor: Pawn отсутствует, сохранён PendingSpawn"));
	}

	return true;
}

// -----------------------------------------------------------------------------
// Pending Spawn Transform API
// -----------------------------------------------------------------------------

void UInteriorSubsystem::SetPendingSpawnTransform(const FVector& Location, const FRotator& Rotation)
{
	bHasPendingSpawn     = true;
	PendingSpawnLocation = Location;
	PendingSpawnRotation = Rotation;
}

void UInteriorSubsystem::GetPendingSpawnTransform(FVector& OutLocation, FRotator& OutRotation) const
{
	OutLocation = PendingSpawnLocation;
	OutRotation = PendingSpawnRotation;
}

void UInteriorSubsystem::ClearPendingSpawnTransform()
{
	bHasPendingSpawn     = false;
	PendingSpawnLocation = FVector::ZeroVector;
	PendingSpawnRotation = FRotator::ZeroRotator;
}

bool UInteriorSubsystem::HasPendingSpawnTransform() const
{
	return bHasPendingSpawn;
}

// -----------------------------------------------------------------------------
// Pending Anchor ID API
// -----------------------------------------------------------------------------

void UInteriorSubsystem::SetPendingAnchorID(const FGuid& AnchorID)  { PendingAnchorID = AnchorID; }
FGuid UInteriorSubsystem::GetPendingAnchorID() const                { return PendingAnchorID; }
void UInteriorSubsystem::ClearPendingAnchorID()                     { PendingAnchorID.Invalidate(); }
bool UInteriorSubsystem::HasPendingAnchorID() const                 { return PendingAnchorID.IsValid(); }

// -----------------------------------------------------------------------------
// Subscribe / Unsubscribe
// -----------------------------------------------------------------------------

void UInteriorSubsystem::SubscribeAll()
{
	UEventBusSubsystem* EventBus = CachedEventBus.Get();
	if (!EventBus) return;

	if (!SpawnRegisterHandle.IsValid())
	{
		SpawnConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		SpawnConditionAsset->OperatorType = EConditionOperator::Composite;
		SpawnConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		SpawnConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		SpawnConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractRegistered;
		SpawnConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		SpawnConditionAsset->CompileCondition();

		if (SpawnConditionAsset->GetCondition().IsValid())
		{
			SpawnRegisterHandle = EventBus->RegisterHandler(
				SpawnConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleInteractRegistration));
		}
	}

	if (!DespawnRegisterHandle.IsValid())
	{
		DespawnConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		DespawnConditionAsset->OperatorType = EConditionOperator::Composite;
		DespawnConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		DespawnConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		DespawnConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractUnregistered;
		DespawnConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		DespawnConditionAsset->CompileCondition();

		if (DespawnConditionAsset->GetCondition().IsValid())
		{
			DespawnRegisterHandle = EventBus->RegisterHandler(
				DespawnConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleInteractRegistration));
		}
	}

	SubscribePlacementRegistration();

	if (!InteractCommandHandle.IsValid())
	{
		InteractCommandConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		InteractCommandConditionAsset->OperatorType = EConditionOperator::Composite;
		InteractCommandConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		InteractCommandConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		InteractCommandConditionAsset->CompileCondition();

		if (InteractCommandConditionAsset->GetCondition().IsValid())
		{
			InteractCommandHandle = EventBus->RegisterHandler(
				InteractCommandConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleInteractCommand));
		}
	}

	if (!SetEnabledHandle.IsValid())
	{
		SetEnabledConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		SetEnabledConditionAsset->OperatorType = EConditionOperator::Composite;
		SetEnabledConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		SetEnabledConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		SetEnabledConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractSetEnabled;
		SetEnabledConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		SetEnabledConditionAsset->CompileCondition();

		if (SetEnabledConditionAsset->GetCondition().IsValid())
		{
			SetEnabledHandle = EventBus->RegisterHandler(
				SetEnabledConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleSetEnabled));
		}
	}

	if (!SetRangeHandle.IsValid())
	{
		SetRangeConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		SetRangeConditionAsset->OperatorType = EConditionOperator::Composite;
		SetRangeConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		SetRangeConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		SetRangeConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractSetRange;
		SetRangeConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		SetRangeConditionAsset->CompileCondition();

		if (SetRangeConditionAsset->GetCondition().IsValid())
		{
			SetRangeHandle = EventBus->RegisterHandler(
				SetRangeConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleSetRange));
		}
	}

	if (!SetTooltipHandle.IsValid())
	{
		SetTooltipConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		SetTooltipConditionAsset->OperatorType = EConditionOperator::Composite;
		SetTooltipConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		SetTooltipConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		SetTooltipConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractSetTooltip;
		SetTooltipConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		SetTooltipConditionAsset->CompileCondition();

		if (SetTooltipConditionAsset->GetCondition().IsValid())
		{
			SetTooltipHandle = EventBus->RegisterHandler(
				SetTooltipConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleSetTooltip));
		}
	}

	if (!FloorStateSaveHandle.IsValid())
	{
		FloorStateSaveConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		FloorStateSaveConditionAsset->OperatorType = EConditionOperator::Composite;
		FloorStateSaveConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		FloorStateSaveConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		FloorStateSaveConditionAsset->FilterRow.InteriorType = EOutcomeInterior::FloorStateSave;
		FloorStateSaveConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		FloorStateSaveConditionAsset->CompileCondition();

		if (FloorStateSaveConditionAsset->GetCondition().IsValid())
		{
			FloorStateSaveHandle = EventBus->RegisterHandler(
				FloorStateSaveConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleFloorStateSave));
		}
	}

	if (!FloorStateRestoreHandle.IsValid())
	{
		FloorStateRestoreConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		FloorStateRestoreConditionAsset->OperatorType = EConditionOperator::Composite;
		FloorStateRestoreConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		FloorStateRestoreConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		FloorStateRestoreConditionAsset->FilterRow.InteriorType = EOutcomeInterior::FloorStateRestore;
		FloorStateRestoreConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		FloorStateRestoreConditionAsset->CompileCondition();

		if (FloorStateRestoreConditionAsset->GetCondition().IsValid())
		{
			FloorStateRestoreHandle = EventBus->RegisterHandler(
				FloorStateRestoreConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleFloorStateRestore));
		}
	}

	if (!FloorTransitionHandle.IsValid())
	{
		FloorTransitionConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		FloorTransitionConditionAsset->OperatorType = EConditionOperator::Composite;
		FloorTransitionConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		FloorTransitionConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		FloorTransitionConditionAsset->FilterRow.InteriorType = EOutcomeInterior::FloorTransition;
		FloorTransitionConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		FloorTransitionConditionAsset->CompileCondition();

		if (FloorTransitionConditionAsset->GetCondition().IsValid())
		{
			FloorTransitionHandle = EventBus->RegisterHandler(
				FloorTransitionConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleFloorTransition));
		}
	}

	// Register handler for mission snapshot release commands (published by MissionSubsystem)
	if (!MissionReleaseHandle.IsValid())
	{
		MissionReleaseConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		MissionReleaseConditionAsset->OperatorType = EConditionOperator::Composite;
		MissionReleaseConditionAsset->FilterRow.OutcomeType = EOutcomeType::Mission;
		MissionReleaseConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		MissionReleaseConditionAsset->CompileCondition();

		if (MissionReleaseConditionAsset->GetCondition().IsValid())
		{
			MissionReleaseHandle = EventBus->RegisterHandler(
				MissionReleaseConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleReleaseMissionSnapshot));
		}
	}

	// Register handler for Update Mission List
	if (!UpdateMissionListHandle.IsValid())
	{
		UpdateMissionListConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		UpdateMissionListConditionAsset->OperatorType = EConditionOperator::Composite;
		UpdateMissionListConditionAsset->FilterRow.OutcomeType = EOutcomeType::Mission;
		UpdateMissionListConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		UpdateMissionListConditionAsset->CompileCondition();
		if (UpdateMissionListConditionAsset->GetCondition().IsValid())
		{
			UpdateMissionListHandle = EventBus->RegisterHandler(
				UpdateMissionListConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleUpdateMissionList));
		}
	}
}

void UInteriorSubsystem::SubscribePlacementRegistration()
{
	UEventBusSubsystem* EventBus = CachedEventBus.Get();
	if (!EventBus) return;

	if (!PlacementRegisterHandle.IsValid())
	{
		PlacementRegisterConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		PlacementRegisterConditionAsset->OperatorType = EConditionOperator::Composite;
		PlacementRegisterConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		PlacementRegisterConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		PlacementRegisterConditionAsset->FilterRow.InteriorType = EOutcomeInterior::FloorPlacementRegistered;
		PlacementRegisterConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		PlacementRegisterConditionAsset->CompileCondition();

		if (PlacementRegisterConditionAsset->GetCondition().IsValid())
		{
			PlacementRegisterHandle = EventBus->RegisterHandler(
				PlacementRegisterConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandlePlacementRegistration));
		}
	}

	if (!PlacementUnregisterHandle.IsValid())
	{
		PlacementUnregisterConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		PlacementUnregisterConditionAsset->OperatorType = EConditionOperator::Composite;
		PlacementUnregisterConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		PlacementUnregisterConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		PlacementUnregisterConditionAsset->FilterRow.InteriorType = EOutcomeInterior::FloorPlacementUnregistered;
		PlacementUnregisterConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		PlacementUnregisterConditionAsset->CompileCondition();

		if (PlacementUnregisterConditionAsset->GetCondition().IsValid())
		{
			PlacementUnregisterHandle = EventBus->RegisterHandler(
				PlacementUnregisterConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandlePlacementRegistration));
		}
	}
}

void UInteriorSubsystem::UnsubscribePlacementRegistration()
{
	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		auto Unreg = [&](FOutcomeHandlerHandle& Handle, UOutcomeConditionAsset*& Asset)
		{
			if (Handle.IsValid()) { EventBus->UnregisterHandler(Handle); Handle.Invalidate(); }
			Asset = nullptr;
		};
		Unreg(PlacementRegisterHandle,   PlacementRegisterConditionAsset);
		Unreg(PlacementUnregisterHandle, PlacementUnregisterConditionAsset);
	}
}

// -----------------------------------------------------------------------------
// Initialize / Deinitialize
// -----------------------------------------------------------------------------

void UInteriorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGameInstance* GI = GetGameInstance())
	{
		CachedEventBus = GI->GetSubsystem<UEventBusSubsystem>();
	}

	BuildAssetIndex();
	SubscribeAll();

	// Подписываемся на загрузку карты (срабатывает после SeamlessTravel)
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UInteriorSubsystem::OnPostLoadMap);
}

void UInteriorSubsystem::Deinitialize()
{
	UnsubscribeAll();

	// Отписываемся от загрузки карты
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	PostLoadMapHandle.Reset();

	RegisteredItems.Empty();
	RegistrationListeners.Empty();
	SpawnedActorsByInteriorFloor.Empty();
	//FloorStateSnapshots.Empty();
	MissionFloorSnapshots.Empty();
	CachedEventBus = nullptr;

	Super::Deinitialize();
}

// Новая функция — обработчик завершения загрузки карты
void UInteriorSubsystem::OnPostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld) return;

	// Проверяем что это игровой мир, а не PIE preview или editor
	if (LoadedWorld->WorldType != EWorldType::Game &&
	    LoadedWorld->WorldType != EWorldType::PIE)
	{
		return;
	}

	if (!HasPendingAnchorID())
	{
		UE_LOG(LogTemp, Verbose, TEXT("InteriorSubsystem::OnPostLoadMap: нет PendingAnchorID"));
		return;
	}

	const FGuid AnchorID = GetPendingAnchorID();

	UE_LOG(LogTemp, Log,
		TEXT("InteriorSubsystem::OnPostLoadMap: карта загружена, ищем якорь [%s]"),
		*AnchorID.ToString());

	// Небольшая задержка — дождаться создания акторов на уровне
	FTimerHandle TimerHandle;
	FTimerDelegate Delegate = FTimerDelegate::CreateLambda([this, AnchorID]()
	{
		// Попытка телепорта — если false, всё равно продолжим проверку (якорь мог отсутствовать, но restore всё равно не делать).
		const bool bOk = TeleportToAnchor(AnchorID);

		// Пытаемся найти актор-якорь в текущем мире
		UWorld* World = GetWorld();
		ALocationAnchorActor* FoundAnchor = nullptr;
		if (World)
		{
			for (TActorIterator<ALocationAnchorActor> It(World); It; ++It)
			{
				if (It->AnchorID == AnchorID)
				{
					FoundAnchor = *It;
					break;
				}
			}
		}

		// Если нашли якорь — извлекаем связанный FloorAsset (если есть) и формируем CurrentKey
		FInteriorFloorKey CurrentKey;
		bool bHaveKey = false;
		if (FoundAnchor)
		{
			UObject* OwnerAsset = FoundAnchor->GetOwnerRegistrationAsset();
			if (UFloorAsset* FloorAsset = Cast<UFloorAsset>(OwnerAsset))
			{
				if (FloorAsset->FloorID.IsValid())
				{
					FGuid InteriorSetId;
					if (FloorAsset->ParentInteriorSet.IsValid() && FloorAsset->ParentInteriorSet.Get())
					{
						InteriorSetId = FloorAsset->ParentInteriorSet.Get()->InteriorSetID;
					}
					CurrentKey = FInteriorFloorKey(InteriorSetId, FloorAsset->FloorID);
					bHaveKey = true;
				}
			}
		}

		// Если есть ключ этажа — восстановим snapshots для всех миссий, у которых он сохранён.
		if (bHaveKey)
		{
			bool bDidRestoreAny = false;

			// 1) Сначала попробуем восстановить mission-snapshots (как раньше)
			if (const TMap<FName, TArray<FFloorSavedActorState>>* PerFloor = MissionFloorSnapshots.Find(CurrentKey))
			{
				if (PerFloor->Num() > 0)
				{
					for (const auto& MissionPair : *PerFloor)
					{
						const FName& MissionId = MissionPair.Key;
						RestoreMissionFloorState(MissionId, CurrentKey);
						UE_LOG(LogTemp, Log,
							TEXT("InteriorSubsystem::OnPostLoadMap: Restored mission snapshot for mission '%s' floor %s/%s"),
							*MissionId.ToString(), *CurrentKey.InteriorSetId.ToString(), *CurrentKey.FloorId.ToString());
						bDidRestoreAny = true;
					}
				}
			}

			// 2) Если mission-snapshots отсутствуют — пытаемся восстановить baseline из FloorStateSnapshots
			if (!bDidRestoreAny)
			{
				if (const TArray<FFloorSavedActorState>* BaseSnap = FloorStateSnapshots.Find(CurrentKey))
				{
					if (BaseSnap && !BaseSnap->IsEmpty())
					{
						UWorld* LocalWorld = GetWorld();
						if (LocalWorld)
						{
							RestoreFromSnapshotArray(LocalWorld, *BaseSnap, CurrentKey.InteriorSetId, CurrentKey.FloorId);
							UE_LOG(LogTemp, Log,
								TEXT("InteriorSubsystem::OnPostLoadMap: Restored baseline FloorStateSnapshots for floor %s/%s (count=%d)"),
								*CurrentKey.InteriorSetId.ToString(), *CurrentKey.FloorId.ToString(), BaseSnap->Num());
							bDidRestoreAny = true;
						}
					}
					else
					{
						UE_LOG(LogTemp, Verbose,
							TEXT("InteriorSubsystem::OnPostLoadMap: FloorStateSnapshots exists but empty for floor %s/%s"),
							*CurrentKey.InteriorSetId.ToString(), *CurrentKey.FloorId.ToString());
					}
				}
				else
				{
					UE_LOG(LogTemp, Verbose,
						TEXT("InteriorSubsystem::OnPostLoadMap: No mission snapshots and no baseline FloorStateSnapshots for floor %s/%s"),
						*CurrentKey.InteriorSetId.ToString(), *CurrentKey.FloorId.ToString());
				}
			}

			// Публикуем уведомление через EventBus о том, что восстановление завершено для этого этажа
			/*
			if (UEventBusSubsystem* EventBus = CachedEventBus.Get() ? CachedEventBus.Get() : GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
			{
				UFloorStatePayload* NotifyP = Cast<UFloorStatePayload>(EventBus->CreatePayload(UFloorStatePayload::StaticClass()));
				if (NotifyP)
				{
					NotifyP->InteriorSetId = CurrentKey.InteriorSetId;
					NotifyP->FloorId = CurrentKey.FloorId;
					// MissionId оставляем пустым — это уведомление
					FOutcomeEventBase NotifyEv;
					NotifyEv.OutcomeType = EOutcomeType::Interior;
					NotifyEv.OutcomeInterior = EOutcomeInterior::FloorStateRestore; // оповещение о завершении рестора
					NotifyEv.Payload = NotifyP;
					EventBus->PublishOutcome(NotifyEv);
				}
			}
			*/
		}

		// Финальная нотификация о завершении загрузки/перехода
		OnTransitionCompleted.Broadcast(bOk, TransitionPayloadCache ? TransitionPayloadCache->DestinationLink : FLocationAnchorLink(), true);
		ClearPendingAnchorID();
	});

	LoadedWorld->GetTimerManager().SetTimer(TimerHandle, Delegate, 0.5f, false);
}

// --- Missing wrapper implementations required by UHT exec functions ---
// Эти простые реализации связывают публичные UFUNCTION() с внутренней логикой.
// Вставьте этот блок в конец файла (убедитесь, что определений с такими же сигнатурами ещё нет).

void UInteriorSubsystem::SubscribeInteractionRegistration() { SubscribeAll(); }
void UInteriorSubsystem::UnsubscribeInteractionRegistration() { UnsubscribeAll(); }

void UInteriorSubsystem::SubscribeInteractCommand() { SubscribeAll(); }
void UInteriorSubsystem::UnsubscribeInteractCommand() { UnsubscribeAll(); }

void UInteriorSubsystem::SubscribeSetEnabled() { SubscribeAll(); }
void UInteriorSubsystem::UnsubscribeSetEnabled() { UnsubscribeAll(); }

void UInteriorSubsystem::SubscribeSetRange() { SubscribeAll(); }
void UInteriorSubsystem::UnsubscribeSetRange() { UnsubscribeAll(); }

void UInteriorSubsystem::SubscribeSetTooltip() { SubscribeAll(); }
void UInteriorSubsystem::UnsubscribeSetTooltip() { UnsubscribeAll(); }

// Возвращает список размещённых акторов для InteriorSetId/FloorId (const)
TArray<FFloorPopulationRecord> UInteriorSubsystem::GetPlacedActorsForInteriorFloor(const FGuid& InteriorSetId, const FGuid& FloorId) const
{
	TArray<FFloorPopulationRecord> Result;
	const FInteriorFloorKey Key(InteriorSetId, FloorId);
	if (const FFloorPopulationBuckets* Buckets = SpawnedActorsByInteriorFloor.Find(Key))
	{
		Result.Append(Buckets->HeavyFurniture);
		Result.Append(Buckets->LightItems);
		Result.Append(Buckets->Terminals);
		Result.Append(Buckets->NPCSpawners);
		Result.Append(Buckets->Debris);
	}
	return Result;
}

// Add / Remove registration listeners (delegate to FInteractiveSubsystemMethods helpers)
void UInteriorSubsystem::AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
	FInteractiveSubsystemMethods::AddRegistrationListener(ItemId, Listener);
}

void UInteriorSubsystem::RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
	FInteractiveSubsystemMethods::RemoveRegistrationListener(ItemId, Listener);
}

// BuildAssetIndex — если у вас уже есть реализация, можно опустить; иначе простая реализация
void UInteriorSubsystem::BuildAssetIndex()
{
	// Если уже реализовано выше, оставить как есть.
	// Зазиимплементированная версия присутствует в файле; этот вызов может быть просто noop здесь.
	// Оставляем вызов фактической реализации, если она находится в другом месте файла.
	// Если нет — добавьте реализацию аналогично той, что вы видели ранее (AssetRegistry проход).
}

// Реализация защищённого метода, требуемого FInteractiveSubsystemMethods
TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& UInteriorSubsystem::GetRegistrationListeners()
{
	return RegistrationListeners;
}

void UInteriorSubsystem::UnsubscribeAll()
{
	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		auto UnregisterHandle = [&](FOutcomeHandlerHandle& Handle, UOutcomeConditionAsset*& Asset)
		{
			if (Handle.IsValid())
			{
				EventBus->UnregisterHandler(Handle);
				Handle.Invalidate();
			}
			Asset = nullptr;
		};

	UnregisterHandle(SpawnRegisterHandle,   SpawnConditionAsset);
		UnregisterHandle(DespawnRegisterHandle, DespawnConditionAsset);

		UnregisterHandle(PlacementRegisterHandle,   PlacementRegisterConditionAsset);
		UnregisterHandle(PlacementUnregisterHandle, PlacementUnregisterConditionAsset);

		UnregisterHandle(InteractCommandHandle, InteractCommandConditionAsset);
		UnregisterHandle(SetEnabledHandle,      SetEnabledConditionAsset);
		UnregisterHandle(SetRangeHandle,        SetRangeConditionAsset);
		UnregisterHandle(SetTooltipHandle,      SetTooltipConditionAsset);

		UnregisterHandle(FloorStateSaveHandle,    FloorStateSaveConditionAsset);
		UnregisterHandle(FloorStateRestoreHandle, FloorStateRestoreConditionAsset);
		UnregisterHandle(FloorTransitionHandle,   FloorTransitionConditionAsset);
	}

	RegisteredItems.Empty();
	RegistrationListeners.Empty();
	// Не очищаем FloorStateSnapshots, SpawnedActorsByInteriorFloor - они должны жить дальше
}

// ─────────────────────────────────────────────────────────────────────────────
// Mission Snapshot API
// ─────────────────────────────────────────────────────────────────────────────

void UInteriorSubsystem::SaveMissionFloorState(FName MissionId, const FInteriorFloorKey& FloorKey)
{
	if (MissionId.IsNone() || !FloorKey.FloorId.IsValid()) return;

	EJobSpacePolicy Policy = EJobSpacePolicy::Freeze;
	TArray<FEnvelopeChannelEntry> Channels;

	SaveFloorActorsState(FloorKey.InteriorSetId, FloorKey.FloorId, MissionId, Policy, Channels);

	UE_LOG(LogTemp, Log,
		TEXT("InteriorSubsystem::SaveMissionFloorState: saved for mission '%s' floor %s/%s"),
		*MissionId.ToString(),
		*FloorKey.InteriorSetId.ToString(), *FloorKey.FloorId.ToString());
}

void UInteriorSubsystem::RestoreMissionFloorState(FName MissionId, const FInteriorFloorKey& FloorKey)
{
    if (MissionId.IsNone() || !FloorKey.FloorId.IsValid()) return;

    // Найти per-floor map, затем entry по MissionId
    if (const TMap<FName, TArray<FFloorSavedActorState>>* PerFloor = MissionFloorSnapshots.Find(FloorKey))
    {
        if (const TArray<FFloorSavedActorState>* Snapshot = PerFloor->Find(MissionId))
        {
            if (!Snapshot || Snapshot->IsEmpty()) return;
            UWorld* World = GetWorld();
            if (!World) return;

			auto MissionInterior = ActiveMissions.Find(MissionId);
			auto& Controller = MissionInterior->Controller;
			auto MissionAsset = Controller->GetMissionAsset();
			auto& Envelope = MissionAsset->Envelope;
			if (Envelope.JobSpacePolicy == EJobSpacePolicy::Reset)
				return;

            RestoreFromSnapshotArray(World, *Snapshot, FloorKey.InteriorSetId, FloorKey.FloorId);
            UE_LOG(LogTemp, Log,
                TEXT("InteriorSubsystem::RestoreMissionFloorState: restored snapshot for mission '%s' floor %s/%s"),
                *MissionId.ToString(),
                *FloorKey.InteriorSetId.ToString(), *FloorKey.FloorId.ToString());
        }
    }
}

const TMap<FInteriorFloorKey, TArray<FFloorSavedActorState>>*
UInteriorSubsystem::GetMissionSnapshots(FName MissionId) const
{
	// Сформировать временную карту этаж->snapshot для данной миссии и вернуть указатель на кеш
	MissionSnapshotQueryCache.Reset();
	for (const auto& Pair : MissionFloorSnapshots)
	{
		const FInteriorFloorKey& FloorKey = Pair.Key;
		const TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = Pair.Value;
		if (const TArray<FFloorSavedActorState>* Snap = PerFloor.Find(MissionId))
		{
			MissionSnapshotQueryCache.FindOrAdd(FloorKey) = *Snap;
		}
	}
	return &MissionSnapshotQueryCache;
}

void UInteriorSubsystem::HandleReleaseMissionSnapshot(const FOutcomeEventBase& Outcome)
{
	if (UReleaseMissionSnapshotPayload* P = Cast<UReleaseMissionSnapshotPayload>(Outcome.Payload))
	{
		ReleaseMissionSnapshot(P->MissionId, P->Envelope, P->Policy, P->bIsCompletion);
	}
}

void UInteriorSubsystem::HandleUpdateMissionList(const FOutcomeEventBase& Outcome)
{
	if(UUpdateMissionListPayload* P = Cast<UUpdateMissionListPayload>(Outcome.Payload))
	{
		ActiveMissions.Empty();
		TArray<FName> Keys;
		TMap<FName, FActiveMissionEntry> Map = P->ActiveMissions;
		Map.GetKeys(Keys);
		for (const FName& MissionId : Keys)
		{
			if (!MissionId.IsNone())
			{
				//ActiveMissions.Add(MissionId);
				FActiveMissionInterior MissionInterior;
				
				MissionInterior.MissionId = MissionId;
				MissionInterior.Controller = Map[MissionId].Controller;

				ActiveMissions.Add(MissionId, MissionInterior);
			}
		}
	}
}

void UInteriorSubsystem::ReleaseMissionSnapshot(FName MissionId, const FMissionEnvelope& Envelope, EJobSpacePolicy Policy, bool bIsCompletion /*= false*/)
{
	if (MissionId.IsNone()) return;

	UWorld* World = GetWorld();

	// Если это просто покидание этажа во время миссии — ничего дополнительно не делать:
	if (!bIsCompletion)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("InteriorSubsystem::ReleaseMissionSnapshot: floor-exit for mission '%s', policy=%d — mission snapshot preserved"),
			*MissionId.ToString(), static_cast<int32>(Policy));
		return;
	}

	// Обработка завершения миссии (bIsCompletion == true)
	switch (Policy)
	{
	case EJobSpacePolicy::Reset:
	{
		// ResetAll: восстановить из базового FloorStateSnapshots (если есть)
		for (auto& Pair : MissionFloorSnapshots)
		{
			const FInteriorFloorKey& FloorKey = Pair.Key;
			TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = Pair.Value;
			if (!PerFloor.Contains(MissionId)) continue;

			if (const TArray<FFloorSavedActorState>* BaseSnap = FloorStateSnapshots.Find(FloorKey))
			{
				if (World && BaseSnap && !BaseSnap->IsEmpty())
				{
					RestoreFromSnapshotArray(World, *BaseSnap, FloorKey.InteriorSetId, FloorKey.FloorId);
					UE_LOG(LogTemp, Log,
						TEXT("InteriorSubsystem::ReleaseMissionSnapshot: ResetAll — restored FloorStateSnapshots for floor %s (mission '%s')"),
						*FloorKey.FloorId.ToString(), *MissionId.ToString());
				}
			}
			else
			{
				UE_LOG(LogTemp, Verbose,
					TEXT("InteriorSubsystem::ReleaseMissionSnapshot: ResetAll — no FloorStateSnapshots baseline for floor %s"),
					*FloorKey.FloorId.ToString());
			}
		}
		break;
	}

	case EJobSpacePolicy::Freeze:
	{
		// FreezeAll: копируем mission snapshot в постоянное хранилище FloorStateSnapshots
		for (auto& Pair : MissionFloorSnapshots)
		{
			const FInteriorFloorKey& FloorKey = Pair.Key;
			TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = Pair.Value;
			if (const TArray<FFloorSavedActorState>* MissionSnap = PerFloor.Find(MissionId))
			{
				FloorStateSnapshots.FindOrAdd(FloorKey) = *MissionSnap;
				UE_LOG(LogTemp, Log,
					TEXT("InteriorSubsystem::ReleaseMissionSnapshot: FreezeAll — wrote MissionFloorSnapshot to FloorStateSnapshots for floor %s (mission '%s')"),
					*FloorKey.FloorId.ToString(), *MissionId.ToString());
			}
		}
		break;
	}

	case EJobSpacePolicy::Partial:
	{
		// Partial: применяем политику поканально
		for (auto& Pair : MissionFloorSnapshots)
		{
			const FInteriorFloorKey& FloorKey = Pair.Key;
			TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = Pair.Value;
			const TArray<FFloorSavedActorState>* MissionSnap = PerFloor.Find(MissionId);
			if (!MissionSnap) continue;

			for (const FEnvelopeChannelEntry& ChannelEntry : Envelope.Channels)
			{
				if (ChannelEntry.Policy == EChannelPolicy::Freeze)
				{
					TArray<FFloorSavedActorState>& BaseSnap = FloorStateSnapshots.FindOrAdd(FloorKey);

					for (const FFloorSavedActorState& ActorSnap : *MissionSnap)
					{
						bool bMatchChannel = false;
						if (World)
						{
							for (TActorIterator<AActor> It(World); It; ++It)
							{
								AActor* Actor = *It;
								if (!IsValid(Actor)) continue;
								UFloorAssignmentComponent* FAC = Actor->FindComponentByClass<UFloorAssignmentComponent>();
								if (!FAC) continue;
								if (FAC->ItemId != ActorSnap.ItemId) continue;
								if (FAC->SnapshotChannel == static_cast<ESnapshotChannel>(ChannelEntry.Channel))
								{
									bMatchChannel = true;
								}
								break;
							}
						}
						if (!bMatchChannel) continue;

						bool bUpdated = false;
						for (FFloorSavedActorState& Existing : BaseSnap)
						{
							if (Existing.ItemId == ActorSnap.ItemId)
							{
								Existing = ActorSnap;
								bUpdated = true;
								break;
							}
						}
						if (!bUpdated) BaseSnap.Add(ActorSnap);
					}
				}
				else if (ChannelEntry.Policy == EChannelPolicy::Reset)
				{
					if (const TArray<FFloorSavedActorState>* BaseSnap = FloorStateSnapshots.Find(FloorKey))
					{
						if (World && BaseSnap && !BaseSnap->IsEmpty())
						{
							RestoreFromSnapshotArray(World, *BaseSnap, FloorKey.InteriorSetId, FloorKey.FloorId);
							UE_LOG(LogTemp, Log,
								TEXT("InteriorSubsystem::ReleaseMissionSnapshot: Partial-Reset — restored FloorStateSnapshots for floor %s (mission '%s')"),
								*FloorKey.FloorId.ToString(), *MissionId.ToString());
						}
					}
				}
			}
		}
		break;
	}

	default:
		break;
	}

	// Очищаем mission snapshots для этой миссии
	for (auto It = MissionFloorSnapshots.CreateIterator(); It; ++It)
	{
		TMap<FName, TArray<FFloorSavedActorState>>& PerFloor = It->Value;
		PerFloor.Remove(MissionId);
		if (PerFloor.Num() == 0)
			It.RemoveCurrent();
	}

	UE_LOG(LogTemp, Log,
		TEXT("InteriorSubsystem::ReleaseMissionSnapshot: completed for mission '%s' (policy=%d)"),
		*MissionId.ToString(), static_cast<int32>(Policy));
}

