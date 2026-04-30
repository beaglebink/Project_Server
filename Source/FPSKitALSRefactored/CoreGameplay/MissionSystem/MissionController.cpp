#include "MissionController.h"
#include "MissionSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "Engine/GameInstance.h"
#include "MissionEnvelopePayload.h"

void UMissionController::InitFromAsset(UMissionAsset* InAsset, UGameInstance* InOwner)
{
    MissionAsset        = InAsset;
    OwnerGameInstance   = InOwner;
    Status              = EMissionStatus::Inactive;
    EndReason           = EMissionEndReason::None;
}

void UMissionController::Activate()
{
    if (Status != EMissionStatus::Inactive && Status != EMissionStatus::Suspended)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MissionController[%s]: Activate() called in invalid status %d"),
            *GetMissionId().ToString(), (int32)Status);
        return;
    }

    Status = EMissionStatus::Active;
    //BroadcastStatusChanged(); //Возможно лишний вызов

    OnMissionActivated();

    UE_LOG(LogTemp, Log, TEXT("MissionController[%s]: Activated"), *GetMissionId().ToString());
}

void UMissionController::RequestResolve(EMissionEndReason Reason)
{
    if (Status != EMissionStatus::Active && Status != EMissionStatus::Suspended)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("MissionController[%s]: RequestResolve() called in invalid status %d"),
            *GetMissionId().ToString(), (int32)Status);
        return;
    }

    EndReason = Reason;
    Status    = EMissionStatus::Resolved;

    // Публикуем изменение статуса (для подписчиков)
    //BroadcastStatusChanged();

    // Локальный хук для BP
    OnMissionCompleted(Reason);

    UE_LOG(LogTemp, Log, TEXT("MissionController[%s]: Resolved with reason %d"),
        *GetMissionId().ToString(), (int32)Reason);

    // Делегируем применение политики ExitPolicy через EventBus
    // — MissionSubsystem подписан на события MissionCompleted/MissionFailed/MissionAbandoned
    if (UGameInstance* GI = OwnerGameInstance.Get())
    {
        if (UEventBusSubsystem* EventBus = GI->GetSubsystem<UEventBusSubsystem>())
        {
            UMissionEnvelopePayload* P = Cast<UMissionEnvelopePayload>(
                EventBus->CreatePayload(UMissionEnvelopePayload::StaticClass()));
            if (P)
            {
                // Заполняем payload: MissionId, Asset, EndReason
                P->Setup(GetMissionId(), MissionAsset, Reason);

                FOutcomeEventBase Ev;
                Ev.OutcomeType = EOutcomeType::Mission;

                // Соответствие EndReason -> OutcomeMission
                switch (Reason)
                {
                case EMissionEndReason::Completed:
                    Ev.OutcomeMission = EOutcomeMission::MissionCompleted;
                    break;
                case EMissionEndReason::Failed:
                    Ev.OutcomeMission = EOutcomeMission::MissionFailed;
                    break;
                case EMissionEndReason::Abandoned:
                    Ev.OutcomeMission = EOutcomeMission::MissionAbandoned;
                    break;
                default:
                    Ev.OutcomeMission = EOutcomeMission::Default;
                    break;
                }

                Ev.Payload = P;
                EventBus->PublishOutcome(Ev);

                UE_LOG(LogTemp, Log, TEXT("MissionController: Published mission resolve via EventBus for '%s' (reason=%d)"),
                    *GetMissionId().ToString(), static_cast<int32>(Reason));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("MissionController: Failed to create UMissionEnvelopePayload for '%s'"), *GetMissionId().ToString());
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("MissionController: EventBus subsystem not available to publish mission resolve for '%s'"), *GetMissionId().ToString());
        }
    }
}

void UMissionController::Suspend()
{
    if (Status != EMissionStatus::Active)
    {
        return;
    }
    Status = EMissionStatus::Suspended;
    //BroadcastStatusChanged();
    OnMissionSuspended();

    UE_LOG(LogTemp, Log, TEXT("MissionController[%s]: Suspended"), *GetMissionId().ToString());
}

void UMissionController::Resume()
{
    if (Status != EMissionStatus::Suspended)
    {
        return;
    }
    Status = EMissionStatus::Active;
    //BroadcastStatusChanged();
    OnMissionResumed();

    UE_LOG(LogTemp, Log, TEXT("MissionController[%s]: Resumed"), *GetMissionId().ToString());
}

FName UMissionController::GetMissionId() const
{
    if (MissionAsset)
    {
        return MissionAsset->GetMissionId();
    }
    return NAME_None;
}

const TArray<FMissionEnvelope>& UMissionController::GetEnvelopes() const
{
    static TArray<FMissionEnvelope> EmptyEnvelopes;
    if (MissionAsset)
    {
        return MissionAsset->Envelopes;
    }
    return EmptyEnvelopes;
}
/*
bool UMissionController::HasValidEnvelope() const
{
    if (!MissionAsset) return false;
    return MissionAsset->Envelope.IsValid();
}
*/
void UMissionController::NotifyBuildingExited()
{
    OnBuildingExited();
}

UWorld* UMissionController::GetWorld() const
{
    if (OwnerGameInstance.IsValid())
    {
        return OwnerGameInstance->GetWorld();
    }
    return nullptr;
}
/*
void UMissionController::BroadcastStatusChanged()
{
    UGameInstance* GI = OwnerGameInstance.Get();
    if (!GI) return;

    UEventBusSubsystem* EventBus = GI->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    // Определяем тип события по статусу / причине завершения
    EOutcomeMission MissionOutcome = EOutcomeMission::Default;

    switch (Status)
    {
        case EMissionStatus::Active:
            MissionOutcome = EOutcomeMission::MissionActivated;
            break;
        case EMissionStatus::Resolved:
            switch (EndReason)
            {
                case EMissionEndReason::Completed:  MissionOutcome = EOutcomeMission::MissionCompleted;  break;
                case EMissionEndReason::Failed:     MissionOutcome = EOutcomeMission::MissionFailed;     break;
                case EMissionEndReason::Abandoned:  MissionOutcome = EOutcomeMission::MissionAbandoned;  break;
                default: break;
            }
            break;
        default:
            return; // Не публикуем для Inactive/Suspended/Released
    }

    FOutcomeEventBase Event;
    Event.OutcomeType    = EOutcomeType::Mission;
    Event.OutcomeMission = MissionOutcome;

    // Payload с именем миссии через MissionEnvelopePayload
    Event.Payload = nullptr;
    EventBus->PublishOutcome(Event);
}
*/