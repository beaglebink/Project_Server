#include "MissionSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../SpawnGroupSystem/GhostClearedPayload.h"
#include "MissionProgressPayload.h"
#include "MissionEnvelopePayload.h"
#include "../InteriorInstanceSystem/InteriorSubsystem.h"
#include "../WorldStateSystem/WorldStateSubsystem.h"
#include "../LocationSystem/FloorAsset.h"
#include "../LocationSystem/InteriorSetAsset.h"
#include "../SaveGame/GameSaveSubsystem.h"
#include "JsonObjectConverter.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h" // <- Для TActorIterator
#include "../InteriorInstanceSystem/FloorAssignmentComponent.h" // <- Для UFloorAssignmentComponent

void UMissionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Регистрируемся в GameSaveSubsystem для участия в сохранении/загрузке
	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->RegisterSaveableSubsystem(this);
	}

	// Автоподписка на envelope-события через EventBus
	SubscribeMissionEnvelopeEvents();

	// Lazy subscribe: only subscribe if condition assigned in editor or earlier
	if (GhostClearedCondition)
	{
		SubscribeGhostCleared();
	}
	if (MissionProgressCondition)
	{
		SubscribeMissionProgress();
	}
}

void UMissionSubsystem::Deinitialize()
{
	UnsubscribeMissionEnvelopeEvents();
	UnsubscribeAll();

	// Снимаем регистрацию из GameSaveSubsystem
	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->UnregisterSaveableSubsystem(this);
	}

	Super::Deinitialize();
}

void UMissionSubsystem::SetGhostClearedCondition(UOutcomeConditionAsset* NewCondition)
{
	if (GhostClearedCondition == NewCondition) return;
	GhostClearedCondition = NewCondition;
	if (GhostClearedHandle.IsValid())
	{
		UnsubscribeGhostCleared();
	}
	SubscribeGhostCleared();
}

void UMissionSubsystem::SetMissionProgressCondition(UOutcomeConditionAsset* NewCondition)
{
	if (MissionProgressCondition == NewCondition) return;
	MissionProgressCondition = NewCondition;
	if (MissionProgressHandle.IsValid())
	{
		UnsubscribeMissionProgress();
	}
	SubscribeMissionProgress();
}

void UMissionSubsystem::SubscribeGhostCleared()
{
	if (GhostClearedHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem: Already subscribed to GhostCleared"));
		return;
	}

	if (!GhostClearedCondition)
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem: GhostClearedCondition is null, cannot subscribe"));
		return;
	}

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		GhostClearedHandle = EventBus->RegisterHandler(
			GhostClearedCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::HandleGhostCleared)
		);

		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Subscribed to GhostCleared (handle=%u)"), GhostClearedHandle.GetId());
	}
}

void UMissionSubsystem::SubscribeMissionProgress()
{
	if (MissionProgressHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem: Already subscribed to MissionProgress"));
		return;
	}

	if (!MissionProgressCondition)
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem: MissionProgressCondition is null, cannot subscribe"));
		return;
	}

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		MissionProgressHandle = EventBus->RegisterHandler(
			MissionProgressCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::HandleMissionProgress)
		);

		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Subscribed to MissionProgress (handle=%u)"), MissionProgressHandle.GetId());
	}
}

void UMissionSubsystem::UnsubscribeGhostCleared()
{
	if (!GhostClearedHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		EventBus->UnregisterHandler(GhostClearedHandle);
		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Unsubscribed GhostCleared (handle=%u)"), GhostClearedHandle.GetId());
	}
	GhostClearedHandle.Invalidate();
}

void UMissionSubsystem::UnsubscribeMissionProgress()
{
	if (!MissionProgressHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		EventBus->UnregisterHandler(MissionProgressHandle);
		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Unsubscribed MissionProgress (handle=%u)"), MissionProgressHandle.GetId());
	}
	MissionProgressHandle.Invalidate();
}

void UMissionSubsystem::UnsubscribeAll()
{
	UnsubscribeGhostCleared();
	UnsubscribeMissionProgress();
}

void UMissionSubsystem::HandleGhostCleared(const FOutcomeEventBase& Outcome)
{
	// Defensive check using compiled query (EventBus already filters, this is extra safety)
	// (Защитная проверка с использованием скомпилированного Query (EventBus уже фильтрует))
	if (GhostClearedCondition)
	{
		auto Query = GhostClearedCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome))
		{
			UE_LOG(LogTemp, Verbose, TEXT("MissionSubsystem: Incoming outcome does not satisfy GhostClearedCondition -> ignoring"));
			return;
		}
	}

	if (UGhostClearedPayload* P = Cast<UGhostClearedPayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Ghost %s cleared"), *P->GhostType);
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("MissionSubsystem: HandleGhostCleared called with no payload or unexpected payload type"));
	}

	OnGhostCleared.Broadcast(Outcome);
}

void UMissionSubsystem::HandleMissionProgress(const FOutcomeEventBase& Outcome)
{
	if (MissionProgressCondition)
	{
		auto Query = MissionProgressCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome))
		{
			UE_LOG(LogTemp, Verbose, TEXT("MissionSubsystem: Incoming outcome does not satisfy MissionProgressCondition -> ignoring"));
			return;
		}
	}

	if (UMissionProgressPayload* P = Cast<UMissionProgressPayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Mission %s step %d"),
			*P->MissionName, P->StepIndex);
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("MissionSubsystem: HandleMissionProgress called with no payload or unexpected payload type"));
	}

	OnMissionProgress.Broadcast(Outcome);
}

// ─────────────────────────────────────────────────────────────────────────────
// MISSION LIFECYCLE
// ─────────────────────────────────────────────────────────────────────────────

UMissionController* UMissionSubsystem::CreateMission(UMissionAsset* MissionAsset)
{
	if (!MissionAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem::CreateMission: MissionAsset is null"));
		return nullptr;
	}

	const FName MissionId = MissionAsset->GetMissionId();

	if (ActiveMissions.Contains(MissionId))
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem::CreateMission: Mission '%s' already exists"),
			*MissionId.ToString());
		return ActiveMissions[MissionId].Controller;
	}

	// Проверка конфликтов envelope
	if (MissionAsset->Envelope.IsValid())
	{
		TArray<FEnvelopeConflictInfo> Conflicts = CheckEnvelopeConflicts(MissionId, MissionAsset->Envelope);
		for (const FEnvelopeConflictInfo& Conflict : Conflicts)
		{
			UE_LOG(LogTemp, Log,
				TEXT("MissionSubsystem::CreateMission: Channel conflict — '%s' wins over '%s'"),
				*Conflict.WinnerMissionId.ToString(), *Conflict.LoserMissionId.ToString());
		}
	}

	UMissionController* Controller = NewObject<UMissionController>(GetGameInstance());
	Controller->InitFromAsset(MissionAsset, GetGameInstance());

	FActiveMissionEntry Entry;
	Entry.MissionId   = MissionId;
	Entry.Controller  = Controller;
	ActiveMissions.Add(MissionId, Entry);

	UE_LOG(LogTemp, Log, TEXT("MissionSubsystem::CreateMission: Created mission '%s'"), *MissionId.ToString());
	return Controller;
}

void UMissionSubsystem::ActivateMission(FName MissionId)
{
	FActiveMissionEntry* Entry = ActiveMissions.Find(MissionId);
	if (!Entry || !Entry->Controller)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("MissionSubsystem::ActivateMission: Mission '%s' not found"), *MissionId.ToString());
		return;
	}

	// Сохранить snapshot состояния всех этажей в scope envelope
	if (UInteriorSubsystem* Interior = GetGameInstance()->GetSubsystem<UInteriorSubsystem>())
	{
		const FMissionEnvelope& Envelope = Entry->Controller->GetEnvelope();
		for (const TSoftObjectPtr<UFloorAsset>& FloorRef : Envelope.Scope.InteriorScopes)
		{
			if (UFloorAsset* Floor = FloorRef.Get())
			{
				FGuid SetId;
				if (Floor->ParentInteriorSet.IsValid() && Floor->ParentInteriorSet.Get())
				{
					SetId = Floor->ParentInteriorSet.Get()->InteriorSetID;
				}
				if (Floor->FloorID.IsValid())
				{
					Interior->SaveMissionFloorState(MissionId, FInteriorFloorKey(SetId, Floor->FloorID));
				}
			}
		}
	}

	Entry->Controller->Activate();
}

void UMissionSubsystem::ResolveMission(FName MissionId, EMissionEndReason Reason)
{
	FActiveMissionEntry* Entry = ActiveMissions.Find(MissionId);
	if (!Entry || !Entry->Controller)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("MissionSubsystem::ResolveMission: Mission '%s' not found"), *MissionId.ToString());
		return;
	}

	Entry->Controller->RequestResolve(Reason);
	ApplyEnvelopeExitPolicy(MissionId, Entry->Controller->GetEnvelope());
}

UMissionController* UMissionSubsystem::GetMissionController(FName MissionId) const
{
	const FActiveMissionEntry* Entry = ActiveMissions.Find(MissionId);
	return Entry ? Entry->Controller : nullptr;
}

bool UMissionSubsystem::IsMissionActive(FName MissionId) const
{
	const FActiveMissionEntry* Entry = ActiveMissions.Find(MissionId);
	if (!Entry || !Entry->Controller) return false;
	const EMissionStatus S = Entry->Controller->GetStatus();
	return S == EMissionStatus::Active || S == EMissionStatus::Suspended;
}

TArray<FName> UMissionSubsystem::GetActiveMissionIds() const
{
	TArray<FName> Result;
	for (const auto& Pair : ActiveMissions)
	{
		Result.Add(Pair.Key);
	}
	return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
// CONFLICT RESOLUTION
// ─────────────────────────────────────────────────────────────────────────────

TArray<FEnvelopeConflictInfo> UMissionSubsystem::CheckEnvelopeConflicts(
	FName NewMissionId,
	const FMissionEnvelope& NewEnvelope) const
{
	TArray<FEnvelopeConflictInfo> Result;

	for (const auto& Pair : ActiveMissions)
	{
		if (Pair.Key == NewMissionId) continue;
		const UMissionController* OtherCtrl = Pair.Value.Controller;
		if (!OtherCtrl) continue;

		const FMissionEnvelope& OtherEnvelope = OtherCtrl->GetEnvelope();

		if (!ScopesOverlap(NewEnvelope.Scope, OtherEnvelope.Scope)) continue;
		if (!ChannelsOverlap(NewEnvelope, OtherEnvelope)) continue;

		FEnvelopeConflictInfo Info;
		Info.bHasConflict = true;

		// Меньший Priority = победитель
		if (NewEnvelope.Priority <= OtherEnvelope.Priority)
		{
			Info.WinnerMissionId = NewMissionId;
			Info.LoserMissionId  = Pair.Key;
		}
		else
		{
			Info.WinnerMissionId = Pair.Key;
			Info.LoserMissionId  = NewMissionId;
		}
		Result.Add(Info);
	}

	return Result;
}

FName UMissionSubsystem::GetChannelOwner(
	const FInteriorFloorKey& FloorKey,
	EEnvelopeChannel Channel) const
{
	FName BestOwner = NAME_None;
	int32 BestPriority = INT32_MAX;

	for (const auto& Pair : ActiveMissions)
	{
		const UMissionController* Ctrl = Pair.Value.Controller;
		if (!Ctrl) continue;

		const FMissionEnvelope& Env = Ctrl->GetEnvelope();

		// Проверяем что этаж входит в scope
		bool bInScope = false;
		for (const TSoftObjectPtr<UFloorAsset>& FloorRef : Env.Scope.InteriorScopes)
		{
			if (const UFloorAsset* Floor = FloorRef.Get())
			{
				if (Floor->FloorID == FloorKey.FloorId)
				{
					bInScope = true;
					break;
				}
			}
		}
		if (!bInScope) continue;

		// Проверяем что канал объявлен
		TOptional<EChannelPolicy> Policy = Env.GetPolicyForChannel(Channel);
		if (!Policy.IsSet() && Env.JobSpacePolicy == EJobSpacePolicy::Partial) continue;

		if (Env.Priority < BestPriority)
		{
			BestPriority = Env.Priority;
			BestOwner    = Pair.Key;
		}
	}

	return BestOwner;
}

bool UMissionSubsystem::ScopesOverlap(
	const FMissionEnvelopeScope& A,
	const FMissionEnvelopeScope& B) const
{
	// Здания должны совпадать
	if (!A.BuildingDisplayName.EqualTo(B.BuildingDisplayName)) return false;

	// Хотя бы один этаж должен быть общим
	for (const TSoftObjectPtr<UFloorAsset>& FloorA : A.InteriorScopes)
	{
		for (const TSoftObjectPtr<UFloorAsset>& FloorB : B.InteriorScopes)
		{
			if (FloorA == FloorB) return true;
		}
	}
	return false;
}

bool UMissionSubsystem::ChannelsOverlap(
	const FMissionEnvelope& A,
	const FMissionEnvelope& B) const
{
	// Если у обоих политика не Partial — любые каналы потенциально конфликтуют
	if (A.JobSpacePolicy != EJobSpacePolicy::Partial ||
		B.JobSpacePolicy != EJobSpacePolicy::Partial)
	{
		return true;
	}

	// Partial — проверяем явное пересечение каналов
	for (const FEnvelopeChannelEntry& EntryA : A.Channels)
	{
		for (const FEnvelopeChannelEntry& EntryB : B.Channels)
		{
			if (EntryA.Channel == EntryB.Channel) return true;
		}
	}
	return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// BUILDING EXIT
// ─────────────────────────────────────────────────────────────────────────────

void UMissionSubsystem::NotifyBuildingExited(const FText& BuildingDisplayName)
{
	for (auto& Pair : ActiveMissions)
	{
		UMissionController* Ctrl = Pair.Value.Controller;
		if (!Ctrl) continue;

		const FMissionEnvelope& Env = Ctrl->GetEnvelope();
		if (Env.Scope.BuildingDisplayName.EqualTo(BuildingDisplayName))
		{
			Ctrl->NotifyBuildingExited();

			// Применить политику выхода согласно ExitPolicy.OnLeaveBuilding
			ApplyEnvelopeExitPolicy(Pair.Key, Env);
		}
	}
}

void UMissionSubsystem::ApplyEnvelopeExitPolicy(FName MissionId, const FMissionEnvelope& Envelope)
{
	const EJobSpacePolicy Policy = Envelope.ExitPolicy.OnLeaveBuilding;

	if (Policy == EJobSpacePolicy::None)
	{
		// Поведение мира по умолчанию — ничего не делаем
		return;
	}

	// Для Partial — проверяем каналы с политикой Persist и промоутим в WorldState
	if (Policy == EJobSpacePolicy::Partial)
	{
		for (const FEnvelopeChannelEntry& Ch : Envelope.Channels)
		{
			if (Ch.Policy == EChannelPolicy::Persist)
			{
				PromoteChannelToWorldState(MissionId, Envelope);
				break;
			}
		}
	}

	// InteriorSubsystem применяет политику: Reset сбрасывает, остальное оставляет
	if (UInteriorSubsystem* Interior = GetGameInstance()->GetSubsystem<UInteriorSubsystem>())
	{
		Interior->ReleaseMissionSnapshot(MissionId, Envelope, Policy);
	}
}

void UMissionSubsystem::PromoteChannelToWorldState(
	FName MissionId,
	const FMissionEnvelope& Envelope)
{
	UWorldStateSubsystem* WSS = GetGameInstance()->GetSubsystem<UWorldStateSubsystem>();
	UInteriorSubsystem*   IS  = GetGameInstance()->GetSubsystem<UInteriorSubsystem>();
	if (!WSS || !IS) return;

	// Получаем mission-snapshot из InteriorSubsystem и переносим
	// SaveGame-свойства Persist-объектов в WorldStateSubsystem.
	const TMap<FInteriorFloorKey, TArray<FFloorSavedActorState>>* Snapshots =
		IS->GetMissionSnapshots(MissionId);
	if (!Snapshots) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// Собираем карту ItemId -> Actor через FloorAssignmentComponent
	TMap<FGuid, AActor*> ActorByItemId;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor)) continue;
		if (UFloorAssignmentComponent* FAC =
			Actor->FindComponentByClass<UFloorAssignmentComponent>())
		{
			if (FAC->ItemId.IsValid())
			{
				ActorByItemId.Add(FAC->ItemId, Actor);
			}
		}
	}

	// Итерируемся по этажам в scope
	for (const auto& FloorPair : *Snapshots)
	{
		for (const FFloorSavedActorState& Snapshot : FloorPair.Value)
		{
			AActor** ActorPtr = ActorByItemId.Find(Snapshot.ItemId);
			if (!ActorPtr || !IsValid(*ActorPtr)) continue;

			AActor* Actor = *ActorPtr;
			UFloorAssignmentComponent* FAC =
				Actor->FindComponentByClass<UFloorAssignmentComponent>();
			if (!FAC) continue;

			// Проверяем что канал этого актора входит в Persist-политику Envelope
			EEnvelopeChannel ActorChannel = FloorActorTypeToEnvelopeChannel(FAC->ActorType);
			TOptional<EChannelPolicy> Policy = Envelope.GetPolicyForChannel(ActorChannel);

			bool bShouldPersist = false;
			if (Envelope.JobSpacePolicy != EJobSpacePolicy::Partial)
			{
				// Для Reset/Freeze/None — не промоутим
				bShouldPersist = false;
			}
			else if (Policy.IsSet() &&
				(Policy.GetValue() == EChannelPolicy::Persist ||
				 Policy.GetValue() == EChannelPolicy::PersistIdentityOnly))
			{
				bShouldPersist = true;
			}

			if (!bShouldPersist) continue;

			// Снимаем текущие SaveGame-свойства с актора и пишем в WorldStateSubsystem
			for (TFieldIterator<FProperty> PropIt(Actor->GetClass()); PropIt; ++PropIt)
			{
				FProperty* Prop = *PropIt;
				if (!Prop->HasAllPropertyFlags(CPF_SaveGame)) continue;

				FString Exported;
				Prop->ExportText_InContainer(0, Exported, Actor, nullptr, Actor, PPF_None);

				FWorldStateRecord Record(
					FAC->ItemId,
					EWorldStateChangeCategory::InteractiveObject,
					Prop->GetFName(),
					Exported,
					MissionId
				);
				WSS->SetWorldStateRecord(Record);
			}
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("MissionSubsystem: PromoteChannelToWorldState done for mission '%s'"),
		*MissionId.ToString());
}

// ─────────────────────────────────────────────────────────────────────────────
// EventBus envelope events
// ─────────────────────────────────────────────────────────────────────────────

void UMissionSubsystem::SubscribeMissionEnvelopeEvents()
{
	UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus) return;

	// Регистрируемся только если ассет условия задан (назначается из редактора)
	auto TryRegister = [&](TObjectPtr<UOutcomeConditionAsset>& Condition,
	                       FOutcomeHandlerHandle& Handle,
	                       auto HandlerMethod)
	{
		if (!Condition || Handle.IsValid()) return;
		if (!Condition->GetCondition().IsValid())
		{
			Condition->CompileCondition();
		}
		if (!Condition->GetCondition().IsValid()) return;
		Handle = EventBus->RegisterHandler(
			Condition,
			FOutcomeHandlerDelegate::CreateUObject(this, HandlerMethod));
	};

	TryRegister(MissionActivatedCondition, EnvelopeActivateHandle,
		&UMissionSubsystem::HandleEnvelopeActivate);
	TryRegister(MissionResolvedCondition,  EnvelopeResolveHandle,
		&UMissionSubsystem::HandleEnvelopeResolve);
	TryRegister(BuildingLeavingCondition,  BuildingLeavingHandle,
		&UMissionSubsystem::HandleBuildingLeaving);

	UE_LOG(LogTemp, Log,
		TEXT("MissionSubsystem: SubscribeMissionEnvelopeEvents (Activate=%d, Resolve=%d, Building=%d)"),
		EnvelopeActivateHandle.IsValid(), EnvelopeResolveHandle.IsValid(), BuildingLeavingHandle.IsValid());
}

void UMissionSubsystem::UnsubscribeMissionEnvelopeEvents()
{
	UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus) return;

	auto Unreg = [&](FOutcomeHandlerHandle& Handle)
	{
		if (Handle.IsValid()) { EventBus->UnregisterHandler(Handle); Handle.Invalidate(); }
	};

	Unreg(EnvelopeActivateHandle);
	Unreg(EnvelopeResolveHandle);
	Unreg(BuildingLeavingHandle);
}

void UMissionSubsystem::HandleEnvelopeActivate(const FOutcomeEventBase& Outcome)
{
	// Принимаем только если это MissionActivated с нашим Payload
	if (Outcome.OutcomeMission != EOutcomeMission::MissionActivated) return;

	UMissionEnvelopePayload* P = Cast<UMissionEnvelopePayload>(Outcome.Payload);
	if (!P || !P->MissionAsset) return;

	const FName MissionId = P->MissionAsset->GetMissionId();

	// Если миссия уже существует — игнорируем (дублированное событие)
	if (ActiveMissions.Contains(MissionId)) return;

	UMissionController* Ctrl = CreateMission(P->MissionAsset);
	if (Ctrl)
	{
		ActivateMission(MissionId);
		UE_LOG(LogTemp, Log,
			TEXT("MissionSubsystem: Mission '%s' created+activated via EventBus"),
			*MissionId.ToString());
	}
}

void UMissionSubsystem::HandleEnvelopeResolve(const FOutcomeEventBase& Outcome)
{
	// Ловим только события завершения миссии
	EMissionEndReason Reason = EMissionEndReason::None;
	switch (Outcome.OutcomeMission)
	{
	case EOutcomeMission::MissionCompleted: Reason = EMissionEndReason::Completed; break;
	case EOutcomeMission::MissionFailed:    Reason = EMissionEndReason::Failed;    break;
	case EOutcomeMission::MissionAbandoned: Reason = EMissionEndReason::Abandoned; break;
	default: return; // Не наш тип события
	}

	UMissionEnvelopePayload* P = Cast<UMissionEnvelopePayload>(Outcome.Payload);
	if (!P) return;

	const FName MissionId = P->MissionId;
	if (MissionId.IsNone()) return;

	// Не обрабатываем если уже resolved (избегаем двойного вызова —
	// BroadcastStatusChanged из Controller сам публикует событие)
	const FActiveMissionEntry* Entry = ActiveMissions.Find(MissionId);
	if (!Entry || !Entry->Controller) return;
	if (Entry->Controller->GetStatus() == EMissionStatus::Resolved) return;

	ResolveMission(MissionId, Reason);

	UE_LOG(LogTemp, Log,
		TEXT("MissionSubsystem: Mission '%s' resolved via EventBus (reason=%d)"),
		*MissionId.ToString(), static_cast<int32>(Reason));
}

void UMissionSubsystem::HandleBuildingLeaving(const FOutcomeEventBase& Outcome)
{
	// Payload должен содержать DisplayName здания — используем InteriorTransitionPayload
	// Если payload не нашёлся — ищем BuildingDisplayName в общем MissionEnvelopePayload
	if (UMissionEnvelopePayload* P = Cast<UMissionEnvelopePayload>(Outcome.Payload))
	{
		// Blueprint мог положить BuildingDisplayName в Payload через другой тип;
		// для этого случая используем прямой вызов
		return;
	}

	// Стандартный путь: InteriorSubsystem публикует BuildingLeaving с InteriorTransitionPayload
	// и одновременно вызывает NotifyBuildingExited напрямую.
	// Этот обработчик — дополнительный канал для Blueprint-triggered события.
	UE_LOG(LogTemp, Verbose,
		TEXT("MissionSubsystem: BuildingLeaving event received (no MissionEnvelopePayload)"));
}

// ─────────────────────────────────────────────────────────────────────────────
// ISaveableSubsystem
// ─────────────────────────────────────────────────────────────────────────────

// ─── Вспомогательные структуры для JSON-сериализации миссий ──────────────────

struct FMissionSaveEntry
{
	FString MissionId;
	FString AssetPath;
	uint8   Status   = 0;
	uint8   EndReason = 0;
	uint8   ResumeMode = 0;
};

void UMissionSubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
	OutData.SubsystemName = GetSaveSubsystemName();

	// Сериализуем список активных миссий в JSON-строку
	TArray<TSharedPtr<FJsonValue>> MissionArray;

	for (const auto& Pair : ActiveMissions)
	{
		const UMissionController* Ctrl = Pair.Value.Controller;
		if (!Ctrl) continue;

		const UMissionAsset* Asset = Ctrl->GetMissionAsset();
		if (!Asset) continue;

		const EMissionStatus Status      = Ctrl->GetStatus();
		const EMissionEndReason EndReason = Ctrl->GetEndReason();
		const EMissionResumeMode Resume  = Asset->Envelope.ResumeMode;

		// Применяем NonResumableMode при сохранении:
		// FailOnLoad — запишем статус как Failed
		// RestartOnLoad — запишем специальный маркер для перезапуска при загрузке
		uint8 SavedStatus    = static_cast<uint8>(Status);
		uint8 SavedEndReason = static_cast<uint8>(EndReason);

		if (Resume == EMissionResumeMode::FailOnLoad &&
			Status == EMissionStatus::Active)
		{
			SavedStatus    = static_cast<uint8>(EMissionStatus::Resolved);
			SavedEndReason = static_cast<uint8>(EMissionEndReason::Failed);
		}
		// RestartOnLoad — оставляем статус, при LoadGame MissionSubsystem перезапустит

		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("MissionId"), Pair.Key.ToString());
		Obj->SetStringField(TEXT("AssetPath"), Asset->GetPathName());
		Obj->SetNumberField(TEXT("Status"),    SavedStatus);
		Obj->SetNumberField(TEXT("EndReason"), SavedEndReason);
		Obj->SetNumberField(TEXT("ResumeMode"), static_cast<uint8>(Resume));

		MissionArray.Add(MakeShared<FJsonValueObject>(Obj));
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetArrayField(TEXT("Missions"), MissionArray);

	FString Output;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	OutData.SerializedData = Output;
}

void UMissionSubsystem::ApplySaveData(const FSubsystemSaveData& InData)
{
	if (InData.SerializedData.IsEmpty()) return;

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InData.SerializedData);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return;

	const TArray<TSharedPtr<FJsonValue>>* MissionArray = nullptr;
	if (!Root->TryGetArrayField(TEXT("Missions"), MissionArray)) return;

	for (const TSharedPtr<FJsonValue>& Val : *MissionArray)
	{
		const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
		if (!Val->TryGetObject(ObjPtr)) continue;
		const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

		FString MissionIdStr, AssetPath;
		int32 StatusInt = 0, EndReasonInt = 0, ResumeModeInt = 0;

		Obj->TryGetStringField(TEXT("MissionId"), MissionIdStr);
		Obj->TryGetStringField(TEXT("AssetPath"), AssetPath);
		Obj->TryGetNumberField(TEXT("Status"),    StatusInt);
		Obj->TryGetNumberField(TEXT("EndReason"), EndReasonInt);
		Obj->TryGetNumberField(TEXT("ResumeMode"), ResumeModeInt);

		const FName MissionId = FName(*MissionIdStr);
		const EMissionStatus   Status    = static_cast<EMissionStatus>(StatusInt);
		const EMissionEndReason EndReason = static_cast<EMissionEndReason>(EndReasonInt);
		const EMissionResumeMode Resume  = static_cast<EMissionResumeMode>(ResumeModeInt);

		// Загрузить ассет
		UMissionAsset* Asset = Cast<UMissionAsset>(
			StaticLoadObject(UMissionAsset::StaticClass(), nullptr, *AssetPath));
		if (!Asset)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("MissionSubsystem::ApplySaveData: Cannot load MissionAsset '%s'"), *AssetPath);
			continue;
		}

		// RestartOnLoad — создать заново и активировать
		if (Resume == EMissionResumeMode::RestartOnLoad)
		{
			UMissionController* Ctrl = CreateMission(Asset);
			if (Ctrl) ActivateMission(MissionId);
			continue;
		}

		// FailOnLoad — запись уже сохранена с Failed статусом, просто создаём запись
		// Resumable — восстанавливаем как было
		UMissionController* Ctrl = CreateMission(Asset);
		if (!Ctrl) continue;

		// Восстанавливаем статус из сохранения
		if (Status == EMissionStatus::Active || Status == EMissionStatus::Suspended)
		{
			Ctrl->Activate();
			if (Status == EMissionStatus::Suspended)
			{
				Ctrl->Suspend();
			}
		}
		else if (Status == EMissionStatus::Resolved)
		{
			Ctrl->Activate();
			Ctrl->RequestResolve(EndReason);
		}

		UE_LOG(LogTemp, Log,
			TEXT("MissionSubsystem::ApplySaveData: Restored mission '%s' Status=%d"),
			*MissionIdStr, StatusInt);
	}
}

