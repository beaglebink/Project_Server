#include "MissionController.h"
#include "MissionSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "Engine/GameInstance.h"
#include "MissionEnvelopePayload.h"
#include <MissionProgressPayload.h>

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

void UMissionController::OnRestore()
{
    OnMissionRestored();

    UE_LOG(LogTemp, Log, TEXT("MissionController[%s]: Restored"), *GetMissionId().ToString());
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

void UMissionController::PublishMissionProgress(int32 StepIndex)
{
    UGameInstance* GI = OwnerGameInstance.Get();
    if (!GI) return;

    UEventBusSubsystem* EventBus = GI->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    UMissionProgressPayload* Payload = EventBus->CreatePayload<UMissionProgressPayload>();
    if (!Payload) return;

    Payload->Setup(GetMissionId(), StepIndex);

    FOutcomeEventBase Event;
    Event.OutcomeType = EOutcomeType::Mission;
    Event.OutcomeMission = EOutcomeMission::MissionProgress;
    Event.Payload = Payload;
    EventBus->PublishOutcome(Event);
}