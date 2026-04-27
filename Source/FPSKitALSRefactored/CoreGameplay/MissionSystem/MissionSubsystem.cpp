#include "MissionSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../SpawnGroupSystem/GhostClearedPayload.h"
#include "MissionProgressPayload.h"
#include "MissionEnvelopePayload.h"
#include "../LocationSystem/FloorAsset.h"
#include "../LocationSystem/InteriorSetAsset.h"
#include "../SaveGame/GameSaveSubsystem.h"
#include "JsonObjectConverter.h"
#include "Engine/GameInstance.h"
#include "../InteriorInstanceSystem/FloorStatePayload.h"
#include "../InteriorInstanceSystem/InteriorTransitionPayload.h"
#include "ReleaseMissionSnapshotPayload.h"

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
	// Подписка на уведомления о покидании этажа (Interior -> Mission coordination)
	SubscribeFloorLeaving();
}

void UMissionSubsystem::Deinitialize()
{
	UnsubscribeMissionEnvelopeEvents();
	UnsubscribeAll();
	UnsubscribeFloorLeaving();

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

void UMissionSubsystem::SubscribeFloorLeaving()
{
	UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus) return;

	if (FloorLeavingHandle.IsValid()) return;

	if (!FloorLeavingCondition)
	{
		FloorLeavingCondition = NewObject<UOutcomeConditionAsset>(this);
		FloorLeavingCondition->OperatorType = EConditionOperator::Composite;
		FloorLeavingCondition->FilterRow.OutcomeType = EOutcomeType::Interior;
		FloorLeavingCondition->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		FloorLeavingCondition->FilterRow.InteriorType = EOutcomeInterior::FloorLeaving;
		FloorLeavingCondition->FilterRow.InteriorComparison = EConditionComparison::Equals;
		FloorLeavingCondition->CompileCondition();
	}

	if (FloorLeavingCondition->GetCondition().IsValid())
	{
		FloorLeavingHandle = EventBus->RegisterHandler(
			FloorLeavingCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::HandleFloorLeavingNotification));
		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Subscribed to FloorLeaving (handle=%u)"), FloorLeavingHandle.IsValid() ? FloorLeavingHandle.GetId() : 0);
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

void UMissionSubsystem::UnsubscribeFloorLeaving()
{
	if (!FloorLeavingHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		EventBus->UnregisterHandler(FloorLeavingHandle);
		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Unsubscribed FloorLeaving (handle=%u)"), FloorLeavingHandle.GetId());
	}
	FloorLeavingHandle.Invalidate();
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

	// Создаём контроллер нужного класса (из ассета). Если в ассете задан Blueprint класс,
	// он будет инстанцирован и вызовы Activate()/OnMissionActivated будут попадать в BP-реализацию.
	UMissionController* Controller = nullptr;
	if (MissionAsset->ControllerClass && MissionAsset->ControllerClass->IsChildOf(UMissionController::StaticClass()))
	{
		Controller = NewObject<UMissionController>(GetGameInstance(), MissionAsset->ControllerClass);
	}
	else
	{
		// Фоллбек на базовый C++ контроллер
		Controller = NewObject<UMissionController>(GetGameInstance());
		if (!MissionAsset->ControllerClass)
		{
			UE_LOG(LogTemp, Verbose, TEXT("MissionSubsystem::CreateMission: ControllerClass not set in MissionAsset '%s', using default UMissionController"), *MissionId.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem::CreateMission: ControllerClass for MissionAsset '%s' is not a UMissionController subclass, using default"), *MissionId.ToString());
		}
	}

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

	// Публикуем FloorStateSave с MissionId для каждого этажа в scope через EventBus
	/*
	UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (EventBus)
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
					UFloorStatePayload* P = Cast<UFloorStatePayload>(
						EventBus->CreatePayload(UFloorStatePayload::StaticClass()));
					if (P)
					{
						P->InteriorSetId = SetId;
						P->FloorId = Floor->FloorID;
						P->MissionId = MissionId;
						FOutcomeEventBase Ev;
						Ev.OutcomeType = EOutcomeType::Interior;
						Ev.OutcomeInterior = EOutcomeInterior::FloorStateSave;
						Ev.Payload = P;
						EventBus->PublishOutcome(Ev);
					}
				}
			}
		}
	}
	*/

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
		return;
	}

	// Для любой политики (Reset/Freeze/Partial) — делегируем через EventBus.
	// InteriorSubsystem обработает Release согласно Policy.
	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		UReleaseMissionSnapshotPayload* P = Cast<UReleaseMissionSnapshotPayload>(
			EventBus->CreatePayload(UReleaseMissionSnapshotPayload::StaticClass()));
		if (P)
		{
			P->Setup(MissionId, Envelope, Policy);
			FOutcomeEventBase Ev;
			Ev.OutcomeType = EOutcomeType::Mission;
			Ev.Payload = P;
			EventBus->PublishOutcome(Ev);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// EventBus envelope events
// ─────────────────────────────────────────────────────────────────────────────

void UMissionSubsystem::SubscribeMissionEnvelopeEvents()
{
	UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus) return;

	// ------- Mission Activated (explicit condition like Interior example) -------
	if (!EnvelopeActivateHandle.IsValid())
	{
		// Если ассет не задан в редакторе — создаём его и настраиваем фильтр
		if (!MissionActivatedCondition)
		{
			MissionActivatedCondition = NewObject<UOutcomeConditionAsset>(this);
			MissionActivatedCondition->OperatorType = EConditionOperator::Composite;
			MissionActivatedCondition->FilterRow.OutcomeType = EOutcomeType::Mission;
			MissionActivatedCondition->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
			MissionActivatedCondition->FilterRow.MissionType = EOutcomeMission::MissionActivated;
			MissionActivatedCondition->FilterRow.MissionComparison = EConditionComparison::Equals;
			MissionActivatedCondition->CompileCondition();
		}

		if (MissionActivatedCondition->GetCondition().IsValid())
		{
			EnvelopeActivateHandle = EventBus->RegisterHandler(
				MissionActivatedCondition,
				FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::HandleEnvelopeActivate));
		}
	}

	// ------- Mission Resolved (filter only by OutcomeType=Mission) -------
	if (!EnvelopeResolveHandle.IsValid())
	{
		if (!MissionResolvedCondition)
		{
			MissionResolvedCondition = NewObject<UOutcomeConditionAsset>(this);
			MissionResolvedCondition->OperatorType = EConditionOperator::Composite;
			MissionResolvedCondition->FilterRow.OutcomeType = EOutcomeType::Mission;
			MissionResolvedCondition->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
			// Не задаём конкретный MissionType — обработчик внутри проверит конкретику (Completed/Failed/Abandoned)
			MissionResolvedCondition->CompileCondition();
		}

		if (MissionResolvedCondition->GetCondition().IsValid())
		{
			EnvelopeResolveHandle = EventBus->RegisterHandler(
				MissionResolvedCondition,
				FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::HandleEnvelopeResolve));
		}
	}

	// ------- Building Leaving (filter by OutcomeType=Mission) -------
	if (!BuildingLeavingHandle.IsValid())
	{
		if (!BuildingLeavingCondition)
		{
			BuildingLeavingCondition = NewObject<UOutcomeConditionAsset>(this);
			BuildingLeavingCondition->OperatorType = EConditionOperator::Composite;
			BuildingLeavingCondition->FilterRow.OutcomeType = EOutcomeType::Mission;
			BuildingLeavingCondition->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
			BuildingLeavingCondition->CompileCondition();
		}

		if (BuildingLeavingCondition->GetCondition().IsValid())
		{
			BuildingLeavingHandle = EventBus->RegisterHandler(
				BuildingLeavingCondition,
				FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::HandleBuildingLeaving));
		}
	}

	// ------- Mission Released (published by InteriorSubsystem after ReleaseMissionSnapshot finished) -------
	if (!MissionReleasedHandle.IsValid())
	{
		if (!MissionReleasedCondition)
		{
			MissionReleasedCondition = NewObject<UOutcomeConditionAsset>(this);
			MissionReleasedCondition->OperatorType = EConditionOperator::Composite;
			MissionReleasedCondition->FilterRow.OutcomeType = EOutcomeType::Mission;
			MissionReleasedCondition->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
			// Не задаём конкретный MissionType — в обработчике проверим payload класс
			MissionReleasedCondition->CompileCondition();
		}

		if (MissionReleasedCondition->GetCondition().IsValid())
		{
			MissionReleasedHandle = EventBus->RegisterHandler(
				MissionReleasedCondition,
				FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::HandleMissionReleased));
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("MissionSubsystem: SubscribeMissionEnvelopeEvents (Activate=%d, Resolve=%d, Building=%d)"),
		EnvelopeActivateHandle.IsValid(), EnvelopeResolveHandle.IsValid(), BuildingLeavingHandle.IsValid());
}

// ----------------------------------------------------------------------------- 
// TryRegisterCondition — перенос логики регистрации из лямбды в метод класса
// ----------------------------------------------------------------------------- 
void UMissionSubsystem::TryRegisterCondition(
	TObjectPtr<UOutcomeConditionAsset>& Condition,
	FOutcomeHandlerHandle& Handle,
	void (UMissionSubsystem::* HandlerMethod)(const FOutcomeEventBase&),
	EOutcomeMission MissionFilter)
{
	if (Handle.IsValid()) return;

	UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus) return;

	// Если ассет отсутствует — создаём встроенный ассет и заполняем базовую фильтрацию.
	if (!Condition)
	{
		Condition = NewObject<UOutcomeConditionAsset>(this);
	}

	// Устанавливаем Composite + минимальный фильтр OutcomeType == Mission
	Condition->OperatorType = EConditionOperator::Composite;
	Condition->FilterRow.OutcomeType = EOutcomeType::Mission;
	Condition->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;

	// Для подписки на конкретный тип Mission (например, активация) — задаём MissionType
	if (MissionFilter != EOutcomeMission::Default)
	{
		Condition->FilterRow.MissionType = MissionFilter;
		Condition->FilterRow.MissionComparison = EConditionComparison::Equals;
	}

	// Компиляция и регистрация
	if (!Condition->GetCondition().IsValid())
	{
		Condition->CompileCondition();
	}
	if (!Condition->GetCondition().IsValid()) return;

	Handle = EventBus->RegisterHandler(
		Condition,
		FOutcomeHandlerDelegate::CreateUObject(this, HandlerMethod));
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
	Unreg(MissionReleasedHandle);
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

void UMissionSubsystem::HandleMissionReleased(const FOutcomeEventBase& Outcome)
{
	// Мы ожидаем подтверждение релиза от InteriorSubsystem в виде UReleaseMissionSnapshotPayload
	if (UReleaseMissionSnapshotPayload* P = Cast<UReleaseMissionSnapshotPayload>(Outcome.Payload))
	{
		const FName MissionId = P->MissionId;
		if (MissionId.IsNone()) return;

		// Finalize mission lifecycle: удаляем контроллер и очищаем локальную информацию
		if (FActiveMissionEntry* Entry = ActiveMissions.Find(MissionId))
		{
			UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Finalizing mission release for '%s' (removing controller)"), *MissionId.ToString());
			ActiveMissions.Remove(MissionId);
		}
		else
		{
			UE_LOG(LogTemp, Verbose, TEXT("MissionSubsystem: MissionReleased received for unknown mission '%s'"), *MissionId.ToString());
		}
	}
	else
	{
		// Игнорируем другие payloads
		return;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// ISaveableSubsystem: CollectSaveData / ApplySaveData
// ─────────────────────────────────────────────────────────────────────────────

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

void UMissionSubsystem::HandleFloorLeavingNotification(const FOutcomeEventBase& Outcome)
{
	// Защитная проверка через ассет условия (если назначен)
	if (FloorLeavingCondition)
	{
		auto Query = FloorLeavingCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome)) return;
	}

	// Ожидаем payload типа InteriorTransitionPayload (с SourceFloor)
	UInteriorTransitionPayload* P = Cast<UInteriorTransitionPayload>(Outcome.Payload);
	if (!P) return;

	// Получаем ассет этажа и его идентификаторы
	TSoftObjectPtr<UFloorAsset> SourceFloorRef = P->SourceFloor;
	UFloorAsset* SourceFloor = SourceFloorRef.Get();
	if (!SourceFloor || !SourceFloor->FloorID.IsValid()) return;

	const FGuid FloorId = SourceFloor->FloorID;
	FGuid InteriorSetId;
	if (SourceFloor->ParentInteriorSet.IsValid() && SourceFloor->ParentInteriorSet.Get())
	{
		InteriorSetId = SourceFloor->ParentInteriorSet.Get()->InteriorSetID;
	}

	// Публикуем запросы на сохранение snapshot'ов для всех активных миссий,
	// чей scope включает этот этаж.
	UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus) return;

	for (const auto& Pair : ActiveMissions)
	{
		const FName MissionId = Pair.Key;
		UMissionController* Ctrl = Pair.Value.Controller;
		if (!Ctrl) continue;

		const FMissionEnvelope& Env = Ctrl->GetEnvelope();

		bool bInScope = false;
		for (const TSoftObjectPtr<UFloorAsset>& FloorRef : Env.Scope.InteriorScopes)
		{
			if (UFloorAsset* F = FloorRef.Get())
			{
				if (F->FloorID == FloorId) { bInScope = true; break; }
			}
		}
		if (!bInScope) continue;

		UFloorStatePayload* SaveP = Cast<UFloorStatePayload>(EventBus->CreatePayload(UFloorStatePayload::StaticClass()));
		if (!SaveP) continue;

		SaveP->InteriorSetId = InteriorSetId;
		SaveP->FloorId = FloorId;
		SaveP->MissionId = MissionId;

		FOutcomeEventBase Ev;
		Ev.OutcomeType = EOutcomeType::Interior;
		Ev.OutcomeInterior = EOutcomeInterior::FloorStateSave;
		Ev.Payload = SaveP;

		EventBus->PublishOutcome(Ev);

		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Requested FloorStateSave for mission '%s' floor %s"),
			*MissionId.ToString(), *FloorId.ToString());

		// Переводим контроллер миссии в приостановленное состояние — это сигнал, что миссия временно "поставлена на паузу"
		// Пока игрок уходит с этажа и snapshot сохраняется. Suspend() вызовет BroadcastStatusChanged().
		// Это нужно вызывать не всегда а когда выходим на улицу, потом сделаю
		/*
		if (Ctrl->GetStatus() == EMissionStatus::Active)
		{
			Ctrl->Suspend();
			UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Suspended mission controller for '%s' before leaving floor"), *MissionId.ToString());
		}
		*/
	}

	// После того, как мы запросили сохранение snapshot'ов и приостановили контроллеры,
	// делаем явное сохранение через GameSaveSubsystem — чтобы обеспечить долговременное хранение контроллера/статуса.
	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Triggering SaveGame after FloorLeaving handling"));
		SaveSys->SaveGame();
	}
}

