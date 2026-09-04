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
#include <UpdateMissionListPayload.h>
#include <ApplyMissionCompletionPolicyPayload.h>
#include <UpdateActiveMissionId.h>
#include <CheckRequestPayload.h>
#include <CheckResponsePayload.h>
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// Анонимное пространство имён для вспомогательных функций
namespace
{
	template<typename T>
	bool CompareValues(T Current, T Expected, ECheckCompareOp Op)
	{
		switch (Op)
		{
		case ECheckCompareOp::Equal:          return Current == Expected;
		case ECheckCompareOp::NotEqual:       return Current != Expected;
		case ECheckCompareOp::Less:           return Current < Expected;
		case ECheckCompareOp::LessOrEqual:    return Current <= Expected;
		case ECheckCompareOp::Greater:        return Current > Expected;
		case ECheckCompareOp::GreaterOrEqual: return Current >= Expected;
		default: return false;
		}
	}

	// Функция нормализации имени уровня (убирает PIE-префиксы, приводит к нижнему регистру)
	FString NormalizeLevelName(const FString& InPath)
	{
		if (InPath.IsEmpty()) return FString();
		FString PackagePath = InPath;
		if (PackagePath.Contains(TEXT(".")))
			PackagePath = FPackageName::ObjectPathToPackageName(PackagePath);
		FString Base = FPaths::GetBaseFilename(PackagePath);
		int32 PIEPos = Base.Find(TEXT("UEDPIE_"), ESearchCase::IgnoreCase);
		if (PIEPos != INDEX_NONE)
		{
			int32 MPos = Base.Find(TEXT("_M_"), ESearchCase::IgnoreCase, ESearchDir::FromStart, PIEPos);
			if (MPos != INDEX_NONE && MPos + 3 < Base.Len())
				Base = Base.Mid(MPos + 3);
			else
				Base = Base.Mid(PIEPos + 6);
		}
		return Base.ToLower();
	}
}
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
	SubscribeMissionProgress();
	// Подписка на уведомления о покидании этажа (Interior -> Mission coordination)
	SubscribeFloorLeaving();
	SubscribeRequests();
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

void UMissionSubsystem::SubscribeMissionProgress()
{
	UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus) return;

	if (!MissionProgressHandle.IsValid())
	{
		if (!MissionProgressCondition)
		{
			MissionProgressCondition = NewObject<UOutcomeConditionAsset>(this);
			MissionProgressCondition->OperatorType = EConditionOperator::Composite;
			MissionProgressCondition->FilterRow.OutcomeType = EOutcomeType::Mission;
			MissionProgressCondition->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
			MissionProgressCondition->FilterRow.MissionType = EOutcomeMission::MissionProgress;
			MissionProgressCondition->FilterRow.MissionComparison = EConditionComparison::Equals;
			MissionProgressCondition->CompileCondition();
		}
	}

	if (MissionProgressCondition->GetCondition().IsValid())
	{
		MissionProgressHandle = EventBus->RegisterHandler(
			MissionProgressCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::HandleMissionProgress)
		);
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
	UnsubscribeMissionProgress();
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
		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Mission %s"),
			*P->MissionName.ToString());

		FName MissionName = P->MissionName;


		FActiveMissionEntry* ActiveMission = ActiveMissions.Find(MissionName);
		UMissionController* Controller = ActiveMission ? ActiveMission->Controller : nullptr;

		if(!Controller)
		{
			UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem: No active mission found with name '%s'"), *MissionName.ToString());
			return;
		}

		UMissionAsset* MissionAsset = Controller->GetMissionAsset();
		FName ConflictedMissionName;
		if (MissionAsset)
		{
			if (MissionAsset->Envelopes.IsValidIndex(ActiveMission->MissionStep + 1))
			{
				FMissionEnvelope& Envelope = MissionAsset->Envelopes[ActiveMission->MissionStep + 1];

				if (IsMissionConflict(MissionName, Envelope, ConflictedMissionName))
				{
					UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Mission '%s'  conflicts with '%s'"), *MissionName.ToString(), *ConflictedMissionName.ToString());
					return;
				}
			}

			int32 StepIndex = ActiveMission->MissionStep;
			StepIndex++; // предполагается, что прогресс миссии приходит после успешного выполнения шага, так что индекс шага для получения envelope увеличиваем на 1
			ActiveMission->MissionStep = StepIndex; // сохраняем прогресс миссии (текущий шаг) в MissionSubsystem, чтобы при покидании этажа или загрузке игры можно было восстановить состояние миссии
			ActiveMissions.Add(MissionName, *ActiveMission); // обновляем запись о миссии с новым шагом


			if (MissionAsset->Envelopes.IsValidIndex(StepIndex))
			{
				const FMissionEnvelope& Envelope = MissionAsset->Envelopes[StepIndex];
				
				if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
				{
					UUpdateMissionListPayload* P1 = Cast<UUpdateMissionListPayload>(EventBus->CreatePayload(UUpdateMissionListPayload::StaticClass()));
					//EventBus->CreatePayload(UUpdateMissionListPayload::StaticClass());
					if (P1)
					{
						P1->ActiveMissions = ActiveMissions;
						P1->CurrentMissionId = MissionName;
						P1->Reason = EMissionEndReason::Completed;
						FOutcomeEventBase Ev;
						Ev.OutcomeType = EOutcomeType::Interior;
						Ev.Payload = P1;
						EventBus->PublishOutcome(Ev);
					}
				}

				Controller->MissionStep = StepIndex;
				Controller->OnMissionStepProgress(StepIndex);
			}
			else
			{
				// Если индекс шага выходит за пределы массива Envelopes, это может означать, что миссия завершилась
				UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem: Mission '%s' complete"), *MissionName.ToString());

				ApplyMissionCompletionPolicy(MissionName, MissionAsset->Envelopes.Last(), MissionAsset->Envelopes.Last().NextStagePolicy, EMissionEndReason::Completed);

				Controller->OnMissionCompleted(EMissionEndReason::Completed);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem: No active mission found with name '%s'"), *MissionName.ToString());
		}
	}
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

void UMissionSubsystem::ActivateMission(FName MissionId, bool IsUpdateSnapshots)
{
	UE_LOG(LogTemp, Log, TEXT("ActivateMission: Published MissionActivated for %s"), *MissionId.ToString());

	FActiveMissionEntry* Entry = ActiveMissions.Find(MissionId);
	if (!Entry || !Entry->Controller)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("MissionSubsystem::ActivateMission: Mission '%s' not found"), *MissionId.ToString());
		return;
	}

	Entry->Controller->Activate();

	// --- НОВАЯ ПУБЛИКАЦИЯ ДЛЯ УСЛОВИЙ ---
	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		UMissionEnvelopePayload* Payload = EventBus->CreatePayload<UMissionEnvelopePayload>();
		if (Payload)
		{
			Payload->Setup(MissionId, nullptr, EMissionEndReason::None);
			FOutcomeEventBase Ev;
			Ev.OutcomeType = EOutcomeType::Mission;
			Ev.OutcomeMission = EOutcomeMission::MissionActivated;
			Ev.Payload = Payload;
			EventBus->PublishOutcome(Ev);
		}
	}

	// Существующая публикация (можно оставить)
	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		UUpdateMissionListPayload* P1 = Cast<UUpdateMissionListPayload>(EventBus->CreatePayload(UUpdateMissionListPayload::StaticClass()));
		if (P1)
		{
			P1->ActiveMissions = ActiveMissions;
			P1->CurrentMissionId = MissionId;
			P1->IsUpdateSnapshots = IsUpdateSnapshots;
			P1->Reason = EMissionEndReason::None;
			FOutcomeEventBase Ev;
			Ev.OutcomeType = EOutcomeType::Interior;
			Ev.Payload = P1;
			EventBus->PublishOutcome(Ev);
		}
	}
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

	if (!Entry->Controller->GetEnvelopes().IsValidIndex(Entry->MissionStep))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("MissionSubsystem::ResolveMission: Mission '%s' has no envelope for step %d"), *MissionId.ToString(), Entry->MissionStep);
		return;
	}

	const FMissionEnvelope& Envelope = Entry->Controller->GetEnvelopes()[Entry->MissionStep];

	EJobSpacePolicy ExitPolicy = Envelope.NextStagePolicy;

	if (Reason == EMissionEndReason::Completed)
	{
		// Используем политику завершения миссии (OnMissionCompleted)
		const EJobSpacePolicy CompletionPolicy = Envelope.NextStagePolicy;
		ApplyMissionCompletionPolicy(MissionId, Envelope, CompletionPolicy, Reason);
	}
	else
	{
		// Failed / Abandoned — не обновляем FloorStateSnapshots,
		switch (Reason)
		{
			case EMissionEndReason::Failed:
			{
				ExitPolicy = Envelope.MissionFailedPolicy;
				UE_LOG(LogTemp, Log, TEXT("MissionSubsystem::ResolveMission: Mission '%s' failed"), *MissionId.ToString());
				break;
			}
			case EMissionEndReason::Abandoned:
			{
				ExitPolicy = Envelope.MissionAbandonedPolicy;
				UE_LOG(LogTemp, Log, TEXT("MissionSubsystem::ResolveMission: Mission '%s' abandoned"), *MissionId.ToString());
				break;
			}
		}
		ApplyMissionCompletionPolicy(MissionId, Envelope, ExitPolicy, Reason);
	}
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
	if (A.RuntimePolicy != EJobSpacePolicy::Partial ||
		B.RuntimePolicy != EJobSpacePolicy::Partial)
	{
		return true;
	}

	// Partial — проверяем явное пересечение каналов
	for (const FEnvelopeChannelEntry& EntryA : A.RuntimePolicyChannels)
	{
		for (const FEnvelopeChannelEntry& EntryB : B.RuntimePolicyChannels)
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

		const FMissionEnvelope& Env = Ctrl->GetEnvelopes()[Pair.Value.MissionStep];
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
	const EJobSpacePolicy Policy = Envelope.NextStagePolicy;

	if (Policy == EJobSpacePolicy::None)
	{
		return;
	}

	// Для любой политики (Reset/Freeze/Partial) — делегируем через EventBus.
	// InteriorSubsystem обработает Release согласно Policy.
	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		UReleaseMissionSnapshotPayload* P = Cast<UReleaseMissionSnapshotPayload>(EventBus->CreatePayload(UReleaseMissionSnapshotPayload::StaticClass()));
		if (P)
		{
			P->Setup(MissionId, Envelope, Policy);
			FOutcomeEventBase Ev;
			Ev.OutcomeType = EOutcomeType::Interior;
			Ev.Payload = P;
			EventBus->PublishOutcome(Ev);
		}
	}
}

void UMissionSubsystem::ApplyMissionCompletionPolicy(
	FName MissionId,
	const FMissionEnvelope& Envelope,
	EJobSpacePolicy Policy,
	EMissionEndReason EndReason)
{
	// Дополнительная публикация события MissionCompleted с UMissionEnvelopePayload
	if (EndReason == EMissionEndReason::Completed)
	{
		// Если миссия успешно завершена, добавляем в историю

		CompletedMissionHistory.AddUnique(MissionId);
		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Mission '%s' added to completed history"), *MissionId.ToString());
	}

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		UApplyMissionCompletionPolicyPayload* P = Cast<UApplyMissionCompletionPolicyPayload>(EventBus->CreatePayload(UApplyMissionCompletionPolicyPayload::StaticClass()));
		if (P)
		{
			P->MissionId = MissionId;
			P->Envelope = Envelope;
			P->Policy = Policy;
			P->EndReason = EndReason;

			FOutcomeEventBase Ev;
			Ev.OutcomeType = EOutcomeType::Mission;   
			Ev.OutcomeMission = EOutcomeMission::MissionCompleted;
			Ev.Payload = P;
			EventBus->PublishOutcome(Ev);
		}

		ActiveMissions.Remove(MissionId);
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

	if (!UpdateActiveMissionIdHandle.IsValid())
	{
		// Если ассет не задан в редакторе — создаём его и настраиваем фильтр
		if (!UpdateActiveMissionIdCondition)
		{
			UpdateActiveMissionIdCondition = NewObject<UOutcomeConditionAsset>(this);
			UpdateActiveMissionIdCondition->OperatorType = EConditionOperator::Composite;
			UpdateActiveMissionIdCondition->FilterRow.OutcomeType = EOutcomeType::Mission;
			UpdateActiveMissionIdCondition->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
			UpdateActiveMissionIdCondition->FilterRow.MissionType = EOutcomeMission::MissionUpdate;
			UpdateActiveMissionIdCondition->FilterRow.MissionComparison = EConditionComparison::Equals;
			UpdateActiveMissionIdCondition->CompileCondition();
		}

		if (UpdateActiveMissionIdCondition->GetCondition().IsValid())
		{
			EnvelopeActivateHandle = EventBus->RegisterHandler(
				UpdateActiveMissionIdCondition,
				FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::HandleUpdateActiveMissionId));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: SubscribeActiveMissionId"));
}

void UMissionSubsystem::OnCheckRequest(const FOutcomeEventBase& Event)
{
	UMissionCheckRequestPayload* Req = Cast<UMissionCheckRequestPayload>(Event.Payload);
	if (!Req)
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem: OnCheckRequest - invalid payload"));
		return;
	}

	bool bApproved = false;
	FString Reason;

	// Определяем, действует ли миссия на текущем уровне
	bool bIsActiveOnLevel = IsMissionActiveOnCurrentLevel(Req->MissionId);

	switch (Req->PropertyToCheck)
	{
	case EMissionCheckProperty::IsActive:
	{
		if (Req->DataType != ECheckDataType::Bool)
		{
			Reason = TEXT("DataType mismatch: IsActive expects bool");
			break;
		}
		bool Expected = Req->StringValue.ToBool();
		bool Current = bIsActiveOnLevel; // true только если миссия действует на уровне
		bApproved = CompareValues(Current, Expected, Req->Operator);
		if (!bApproved && Reason.IsEmpty()) Reason = TEXT("Mission active state mismatch");
		break;
	}

	case EMissionCheckProperty::CurrentStep:
	{
		if (Req->DataType != ECheckDataType::Int32)
		{
			Reason = TEXT("DataType mismatch: CurrentStep expects int");
			break;
		}
		// Если миссия не действует на уровне – условие не выполнено
		if (!bIsActiveOnLevel)
		{
			bApproved = false;
			Reason = TEXT("Mission not active on current level");
			break;
		}
		// Миссия действует – проверяем шаг
		UMissionController* Ctrl = GetMissionController(Req->MissionId);
		if (!Ctrl)
		{
			Reason = TEXT("Mission controller not found");
			break;
		}
		int32 Expected = FCString::Atoi(*Req->StringValue);
		int32 Current = Ctrl->MissionStep;
		bApproved = CompareValues(Current, Expected, Req->Operator);
		if (!bApproved && Reason.IsEmpty()) Reason = TEXT("Mission step mismatch");
		break;
	}

	case EMissionCheckProperty::Name:
	{
		if (Req->DataType != ECheckDataType::String)
		{
			Reason = TEXT("DataType mismatch: Name expects string");
			break;
		}
		// Если миссия не действует на уровне – условие не выполнено
		if (!bIsActiveOnLevel)
		{
			bApproved = false;
			Reason = TEXT("Mission not active on current level");
			break;
		}
		// Миссия действует – сравниваем имя
		FString Expected = Req->StringValue;
		FString Current = Req->MissionId.ToString();
		bApproved = CompareValues(Current, Expected, Req->Operator);
		if (!bApproved && Reason.IsEmpty()) Reason = TEXT("Mission name mismatch");
		break;
	}

	default:
		Reason = TEXT("Unsupported property");
		break;
	}

	// Отправляем ответ
	UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus)
	{
		UE_LOG(LogTemp, Error, TEXT("MissionSubsystem: EventBus not available for response"));
		return;
	}

	UCheckResponsePayload* Resp = EventBus->CreatePayload<UCheckResponsePayload>();
	Resp->TransactionId = Req->TransactionId;
	Resp->bApproved = bApproved;
	Resp->Reason = Reason;

	FOutcomeEventBase Reply;
	Reply.OutcomeType = EOutcomeType::Mission;
	Reply.OutcomeMission = EOutcomeMission::CheckResponse;
	Reply.Payload = Resp;
	EventBus->PublishOutcome(Reply);

	UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Responded to check for MissionId=%s, Property=%d, Approved=%s"),
		*Req->MissionId.ToString(), (int32)Req->PropertyToCheck, bApproved ? TEXT("true") : TEXT("false"));
}

bool UMissionSubsystem::IsMissionActiveOnCurrentLevel(FName MissionId) const
{
	if (MissionId.IsNone()) return false;

	const FActiveMissionEntry* Entry = ActiveMissions.Find(MissionId);
	if (!Entry || !Entry->Controller) return false;

	if (!IsMissionActive(MissionId)) return false;

	UMissionAsset* MissionAsset = Entry->Controller->GetMissionAsset();
	if (!MissionAsset || !MissionAsset->Envelopes.IsValidIndex(Entry->MissionStep)) return false;

	const FMissionEnvelope& Envelope = MissionAsset->Envelopes[Entry->MissionStep];

	UWorld* World = GetWorld();
	if (!World) return false;

	FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);
	FString NormCurrent = NormalizeLevelName(CurrentLevelName);

	for (const TSoftObjectPtr<UFloorAsset>& FloorRef : Envelope.Scope.InteriorScopes)
	{
		if (UFloorAsset* Floor = FloorRef.Get())
		{
			FString ScopeLevelPath = Floor->FloorLevel.ToSoftObjectPath().GetLongPackageName();
			FString NormTarget = NormalizeLevelName(ScopeLevelPath);
			if (NormTarget == NormCurrent)
			{
				return true;
			}
		}
	}
	return false;
}


void UMissionSubsystem::SubscribeRequests()
{
	if (CheckRequestHandle.IsValid()) return; // уже подписаны

	UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus) return;

	// Создаём ConditionAsset для фильтрации запросов
	UOutcomeConditionAsset* Asset = NewObject<UOutcomeConditionAsset>(this);
	Asset->OperatorType = EConditionOperator::Composite;
	Asset->FilterRow.OutcomeType = EOutcomeType::Mission;
	Asset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	Asset->FilterRow.MissionType = EOutcomeMission::CheckRequest;
	Asset->FilterRow.MissionComparison = EConditionComparison::Equals;
	Asset->CompileCondition();

	// Регистрируем обработчик и сохраняем хендл
	CheckRequestHandle = EventBus->RegisterHandler(Asset,
		FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::OnCheckRequest));

	UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Subscribed to CheckRequest (handle=%u)"),
		CheckRequestHandle.IsValid() ? CheckRequestHandle.GetId() : 0);
}

// --- Исправленный UnsubscribeRequests ---
void UMissionSubsystem::UnsubscribeRequests()
{
	if (!CheckRequestHandle.IsValid()) return;

	UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (EventBus)
	{
		EventBus->UnregisterHandler(CheckRequestHandle);
		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Unsubscribed CheckRequest (handle=%u)"),
			CheckRequestHandle.GetId());
	}
	CheckRequestHandle.Invalidate();
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

bool UMissionSubsystem::IsMissionConflict(FName MissionName, FMissionEnvelope NewMissionEnvelope, FName& ConflictedMissionName)
{
	for (auto& AMPair : ActiveMissions)
	{
		if (AMPair.Key == MissionName) continue; // не сравниваем миссию с самой собой

		auto Controller = AMPair.Value.Controller;
		if (!Controller) continue;

		TArray < FMissionEnvelope> Envelopes = Controller->GetEnvelopes();
		if (!Envelopes.IsValidIndex(AMPair.Value.MissionStep)) continue;

		FMissionEnvelope Envelope = Envelopes[AMPair.Value.MissionStep];
		//FMissionEnvelope NewMissionEnvelope = P->MissionAsset->Envelopes[Stage];

		auto Scope = Envelope.Scope;
		TArray<TSoftObjectPtr<class UFloorAsset>> Floors = Scope.InteriorScopes;
		TArray<TSoftObjectPtr<class UFloorAsset>> NewFloors = NewMissionEnvelope.Scope.InteriorScopes;

		bool IsOverlapFloor = false;
		for (auto& NewFloor : NewFloors)
		{
			if (Floors.Contains(NewFloor))
			{
				if ((NewMissionEnvelope.RuntimePolicy != EJobSpacePolicy::Partial || NewMissionEnvelope.RuntimePolicyChannels.Num() > 0) && (Envelope.RuntimePolicy != EJobSpacePolicy::Partial || Envelope.RuntimePolicyChannels.Num() > 0))
				{
					IsOverlapFloor = true;
					ConflictedMissionName = AMPair.Key;
					break;
				}
			}
		}

		if (IsOverlapFloor)
		{
			return true;
		}
	}
	return false;
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

	FName ConflictedMissionName;
	if (!P->MissionAsset->Envelopes.IsValidIndex(0))
	{
		return;
	}

	if (IsMissionConflict(MissionId, P->MissionAsset->Envelopes[0], ConflictedMissionName))
	{
		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Mission '%s'  conflicts with '%s'"), *MissionId.ToString(), *ConflictedMissionName.ToString());
		return;
	}

	UMissionController* Ctrl = CreateMission(P->MissionAsset);
	if (Ctrl)
	{
		ActivateMission(MissionId);
		UE_LOG(LogTemp, Log,
			TEXT("MissionSubsystem: Mission '%s' created+activated via EventBus"),
			*MissionId.ToString());
	}
}

void UMissionSubsystem::HandleUpdateActiveMissionId(const FOutcomeEventBase& Outcome)
{
	if (Outcome.OutcomeMission != EOutcomeMission::MissionUpdate) return;
	if (UUpdateActiveMissionId* P = Cast<UUpdateActiveMissionId>(Outcome.Payload))
	{
		ActiveMissionId = P->ActiveMissionId;
	}
}

void UMissionSubsystem::HandleEnvelopeResolve(const FOutcomeEventBase& Outcome)
{
	if (Outcome.OutcomeMission == EOutcomeMission::MissionActivated)
		return;

	UMissionEnvelopePayload* P = Cast<UMissionEnvelopePayload>(Outcome.Payload);
	if (!P) return;

	const FName MissionId = P->MissionId;
	if (MissionId.IsNone()) return;

	EMissionEndReason Reason = P->EndReason;

	// Не обрабатываем если уже resolved (избегаем двойного вызова —
	// BroadcastStatusChanged из Controller сам публикует событие)
	const FActiveMissionEntry* Entry = ActiveMissions.Find(MissionId);
	if (!Entry || !Entry->Controller) return;

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
	UReleaseMissionSnapshotPayload* P = Cast<UReleaseMissionSnapshotPayload>(Outcome.Payload);
	if (!P) return;

	const FName MissionId = P->MissionId;
	if (MissionId.IsNone()) return;

	// Floor-exit release — снапшот сохранён, но миссия продолжается
	if (!P->bIsCompletion)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("MissionSubsystem: HandleMissionReleased — floor-exит snapshot for '%s', mission continues"),
			*MissionId.ToString());
		return;
	}

	// Только финальный release (MissionCompleted / Failed / Abandoned) — финализируем lifecycle
	if (FActiveMissionEntry* Entry = ActiveMissions.Find(MissionId))
	{
		UE_LOG(LogTemp, Log,
			TEXT("MissionSubsystem: Finalizing mission release for '%s' (removing controller)"),
			*MissionId.ToString());
		ActiveMissions.Remove(MissionId);
	}
	else
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("MissionSubsystem: MissionReleased received for unknown mission '%s'"),
			*MissionId.ToString());
	}

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		UUpdateMissionListPayload* P1 = Cast<UUpdateMissionListPayload>(EventBus->CreatePayload(UUpdateMissionListPayload::StaticClass()));
		//EventBus->CreatePayload(UUpdateMissionListPayload::StaticClass());
		if (P1)
		{
			P1->ActiveMissions = ActiveMissions;
			P1->CurrentMissionId = MissionId;
			P1->Reason = P->EndReason;
			FOutcomeEventBase Ev;
			Ev.OutcomeType = EOutcomeType::Interior;
			Ev.Payload = P1;
			EventBus->PublishOutcome(Ev);
		}
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
		const EMissionResumeMode Resume  = Asset->Envelopes[Pair.Value.MissionStep].ResumeMode;
		const int32 MissionStep = Pair.Value.MissionStep;

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
		Obj->SetNumberField(TEXT("MissionStep"), static_cast<int32>(MissionStep));

		MissionArray.Add(MakeShared<FJsonValueObject>(Obj));
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetArrayField(TEXT("Missions"), MissionArray);

	// Сохраняем историю завершённых миссий
	TArray<TSharedPtr<FJsonValue>> HistoryArray;
	for (const FName& CompletedMission : CompletedMissionHistory)
	{
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("MissionId"), CompletedMission.ToString());
		HistoryArray.Add(MakeShared<FJsonValueObject>(Obj));
	}
	Root->SetArrayField(TEXT("CompletedMissionHistory"), HistoryArray);

	Root->SetStringField(TEXT("ActiveMissionId"), ActiveMissionId.ToString());

	FString Output;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	OutData.SerializedData = Output;
}

void UMissionSubsystem::ApplySaveData(const FSubsystemSaveData& InData)
{
	IsLoadComplete = false;
	// Копируем данные в FString, чтобы лямбда владела своей копией
	FString SerializedDataCopy = InData.SerializedData;

	FTimerHandle TimerHandle;
	FTimerDelegate Delegate = FTimerDelegate::CreateLambda([this, SerializedDataCopy]()
		{
			if (SerializedDataCopy.IsEmpty()) return;

			TSharedPtr<FJsonObject> Root;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(SerializedDataCopy);
			if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return;

			// --- Восстанавливаем ActiveMissionId ---
			FString ActiveMissionIdStr;
			if (Root->TryGetStringField(TEXT("ActiveMissionId"), ActiveMissionIdStr))
			{
				ActiveMissionId = FName(*ActiveMissionIdStr);
				UE_LOG(LogTemp, Log, TEXT("MissionSubsystem::ApplySaveData: Restored ActiveMissionId = '%s'"), *ActiveMissionId.ToString());
			}
			else
			{
				ActiveMissionId = NAME_None;
			}

			const TArray<TSharedPtr<FJsonValue>>* MissionArray = nullptr;
			if (!Root->TryGetArrayField(TEXT("Missions"), MissionArray)) return;

			//EMissionResumeMode RestoredResume = EMissionResumeMode::Resumable;
			ActiveMissions.Empty();
			TArray<FName> DeletedMissions;
			for (const TSharedPtr<FJsonValue>& Val : *MissionArray)
			{
				const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
				if (!Val->TryGetObject(ObjPtr)) continue;
				const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

				FString MissionIdStr, AssetPath;
				int32 StatusInt = 0, EndReasonInt = 0, ResumeModeInt = 0, MissionStep = -1;

				Obj->TryGetStringField(TEXT("MissionId"), MissionIdStr);
				Obj->TryGetStringField(TEXT("AssetPath"), AssetPath);
				Obj->TryGetNumberField(TEXT("Status"), StatusInt);
				Obj->TryGetNumberField(TEXT("EndReason"), EndReasonInt);
				Obj->TryGetNumberField(TEXT("ResumeMode"), ResumeModeInt);
				Obj->TryGetNumberField(TEXT("MissionStep"), MissionStep);

				const FName MissionId = FName(*MissionIdStr);
				const EMissionStatus   Status = static_cast<EMissionStatus>(StatusInt);
				const EMissionEndReason EndReason = static_cast<EMissionEndReason>(EndReasonInt);
				const EMissionResumeMode Resume = static_cast<EMissionResumeMode>(ResumeModeInt);

				UMissionAsset* Asset = Cast<UMissionAsset>(
					StaticLoadObject(UMissionAsset::StaticClass(), nullptr, *AssetPath));
				if (!Asset)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("MissionSubsystem::ApplySaveData: Cannot load MissionAsset '%s'"), *AssetPath);
					continue;
				}

				//RestoredResume = Resume;

				UMissionController* Ctrl = CreateMission(Asset);

				if (Ctrl)
				{
					Ctrl->MissionStep = MissionStep;
				}

				switch (Resume)
				{
				case EMissionResumeMode::Resumable:
				{
					break;
				}
				case EMissionResumeMode::RestartOnLoad:
				{

					break;
				}
				case EMissionResumeMode::FailOnLoad:
				{
					DeletedMissions.Add(MissionId);
					break;
				}
				}

				UE_LOG(LogTemp, Log,
					TEXT("MissionSubsystem::ApplySaveData: Restored mission '%s' Status=%d MissionStep = %d"),
					*MissionIdStr, StatusInt, MissionStep);
			}

			// Восстанавливаем историю завершённых миссий
			const TArray<TSharedPtr<FJsonValue>>* HistoryArray = nullptr;
			if (Root->TryGetArrayField(TEXT("CompletedMissionHistory"), HistoryArray))
			{
				CompletedMissionHistory.Empty();
				for (const TSharedPtr<FJsonValue>& Val : *HistoryArray)
				{
					const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
					if (!Val->TryGetObject(ObjPtr)) continue;
					const TSharedPtr<FJsonObject>& Obj = *ObjPtr;
					FString MissionIdStr;
					Obj->TryGetStringField(TEXT("MissionId"), MissionIdStr);
					if (!MissionIdStr.IsEmpty())
						CompletedMissionHistory.Add(FName(*MissionIdStr));
				}
			}
			
			for (auto DelMissionName : DeletedMissions)
			{
				ActiveMissions.Remove(DelMissionName);
			}

			IsLoadComplete = true;
		});

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TimerHandle, Delegate, 0.5f, false);
	}
}

bool UMissionSubsystem::IsMissionCompleted(FName MissionId) const
{
	return CompletedMissionHistory.Contains(MissionId);
}

TArray<FName> UMissionSubsystem::GetCompletedMissionIds() const
{
	return CompletedMissionHistory;
}

void UMissionSubsystem::HandleFloorLeavingNotification(const FOutcomeEventBase& Outcome)
{
	if (FloorLeavingCondition)
	{
		auto Query = FloorLeavingCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome)) return;
	}

	UInteriorTransitionPayload* P = Cast<UInteriorTransitionPayload>(Outcome.Payload);
	if (!P) return;

	TSoftObjectPtr<UFloorAsset> SourceFloorRef = P->SourceFloor;
	UFloorAsset* SourceFloor = SourceFloorRef.Get();
	if (!SourceFloor || !SourceFloor->FloorID.IsValid()) return;

	const FGuid FloorId = SourceFloor->FloorID;
	FGuid InteriorSetId;
	if (SourceFloor->ParentInteriorSet.IsValid() && SourceFloor->ParentInteriorSet.Get())
	{
		InteriorSetId = SourceFloor->ParentInteriorSet.Get()->InteriorSetID;
	}

	UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus) return;

	// Find the highest-priority mission for this floor (larger Priority number = higher priority)
	FName TopMissionId = NAME_None;
	int32 TopPriority = INT32_MIN;

	for (const auto& Pair : ActiveMissions)
	{
		const FName MissionId = Pair.Key;
		UMissionController* Ctrl = Pair.Value.Controller;
		if (!Ctrl) continue;

		const FMissionEnvelope& Env = Ctrl->GetEnvelopes()[Pair.Value.MissionStep];
		bool bInScope = false;
		for (const TSoftObjectPtr<UFloorAsset>& FloorRef : Env.Scope.InteriorScopes)
		{
			if (UFloorAsset* F = FloorRef.Get())
			{
				if (F->FloorID == FloorId) { bInScope = true; break; }
			}
		}
		if (!bInScope) continue;

		const EJobSpacePolicy JobPolicyForSave = Env.RuntimePolicy;
		UFloorStatePayload* SaveP = Cast<UFloorStatePayload>(EventBus->CreatePayload(UFloorStatePayload::StaticClass()));
		if (SaveP)
		{
			SaveP->InteriorSetId = InteriorSetId;
			SaveP->FloorId = FloorId;
			SaveP->MissionId = MissionId;
			SaveP->CurrentMissionStep = Pair.Value.MissionStep;
			SaveP->Channels = Env.RuntimePolicyChannels;
			SaveP->Policy = JobPolicyForSave;
			FOutcomeEventBase SaveEv;
			SaveEv.OutcomeType = EOutcomeType::Interior;
			SaveEv.OutcomeInterior = EOutcomeInterior::FloorStateSave;
			SaveEv.Payload = SaveP;
			EventBus->PublishOutcome(SaveEv);
			UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Saved MissionFloorSnapshot for mission '%s' floor %s"),
				*MissionId.ToString(), *FloorId.ToString());
		}

		TopMissionId = MissionId;
	}

}