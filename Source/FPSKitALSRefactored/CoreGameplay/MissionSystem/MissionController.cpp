#include "MissionController.h"
#include "MissionSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "Engine/GameInstance.h"

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
    BroadcastStatusChanged();

    OnMissionResolved(Reason);

    UE_LOG(LogTemp, Log, TEXT("MissionController[%s]: Resolved with reason %d"),
        *GetMissionId().ToString(), (int32)Reason);
}

void UMissionController::Suspend()
{
    if (Status != EMissionStatus::Active)
    {
        return;
    }
    Status = EMissionStatus::Suspended;
    BroadcastStatusChanged();
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
    BroadcastStatusChanged();
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

const FMissionEnvelope& UMissionController::GetEnvelope() const
{
    static FMissionEnvelope EmptyEnvelope;
    if (MissionAsset)
    {
        return MissionAsset->Envelope;
    }
    return EmptyEnvelope;
}

bool UMissionController::HasValidEnvelope() const
{
    if (!MissionAsset) return false;
    return MissionAsset->Envelope.IsValid();
}

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

    // Payload с именем миссии через MissionProgressPayload
    // (используем существующий тип, не создаём лишних зависимостей)
    EventBus->PublishOutcome(Event);
}
