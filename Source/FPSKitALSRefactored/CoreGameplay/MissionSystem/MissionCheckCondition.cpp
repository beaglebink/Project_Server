// MissionCheckCondition.cpp
#include "MissionCheckCondition.h"
#include "CheckCoordinatorComponent.h"
#include "OutcomeConditionAsset.h"
#include "EventBusSubsystem.h"
#include "MissionSubsystem.h"       // для получения активной миссии
#include "Engine/GameInstance.h"

// ---- Базовый класс ----

UOutcomeConditionAsset* UMissionCheckCondition::CreateSubscriptionCondition() const
{
    UOutcomeConditionAsset* Asset = NewObject<UOutcomeConditionAsset>();
    Asset->OperatorType = EConditionOperator::Composite;
    Asset->FilterRow.OutcomeType = EOutcomeType::Mission;
    Asset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    Asset->FilterRow.MissionType = EOutcomeMission::CheckResponse;
    Asset->FilterRow.MissionComparison = EConditionComparison::Equals;
    Asset->CompileCondition();
    return Asset;
}

FName UMissionCheckCondition::GetActiveMissionId() const
{
    if (!EventBus) return NAME_None;
    // Получаем GameInstance через EventBus (он находится в мире)
    UGameInstance* GI = EventBus->GetGameInstance();
    if (!GI) return NAME_None;
    UMissionSubsystem* MissionSub = GI->GetSubsystem<UMissionSubsystem>();
    if (!MissionSub) return NAME_None;
    return MissionSub->GetActiveMissionId();
}

void UMissionCheckCondition::ExecuteCheck(const FGuid& TransactionId)
{
    if (!EventBus) return;

    // Определяем текущую активную миссию
    FName ActiveMissionId = GetActiveMissionId();
    if (ActiveMissionId.IsNone())
    {
        // Если активной миссии нет – проверка немедленно считается проваленной (или можно пропустить)
        UE_LOG(LogTemp, Warning, TEXT("MissionCheck: No active mission, check failed."));
        bCompleted = true;
        bApproved = false;
        OnComplete.ExecuteIfBound(this);
        return;
    }

    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    UMissionCheckRequestPayload* Payload = CreateRequestPayload();
    if (!Payload)
    {
        UE_LOG(LogTemp, Error, TEXT("MissionCheck: Failed to create payload"));
        return;
    }
    Payload->TransactionId = TransactionId;
    Payload->MissionId = ActiveMissionId;           // автоматически подставляем активную миссию
    Payload->PropertyToCheck = PropertyToCheck;

    FOutcomeEventBase Event;
    Event.OutcomeType = EOutcomeType::Mission;
    Event.OutcomeMission = EOutcomeMission::CheckRequest;
    Event.Payload = Payload;
    EventBus->PublishOutcome(Event);

    UE_LOG(LogTemp, Log, TEXT("MissionCheck: Sent request for MissionId=%s, Property=%d, Txn=%s"),
        *ActiveMissionId.ToString(), (int32)PropertyToCheck, *TransactionId.ToString());

    StartTimeoutTimer(TransactionId);
}

void UMissionCheckCondition::OnCheckResponse(const FOutcomeEventBase& Event)
{
    if (bCompleted) return;

    UCheckResponsePayload* Resp = Cast<UCheckResponsePayload>(Event.Payload);
    if (!Resp || Resp->TransactionId != CurrentTransactionId) return;
    if (Event.OutcomeMission != EOutcomeMission::CheckResponse) return;

    ClearTimeoutTimer();
    bCompleted = true;
    bApproved = Resp->bApproved;

    if (bApproved)
    {
        UE_LOG(LogTemp, Log, TEXT("MissionCheck: Approved"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("MissionCheck: Rejected. Reason: %s"), *Resp->Reason);
    }

    OnComplete.ExecuteIfBound(this);
}

FString UMissionCheckCondition::GetDescription() const
{
    return FString::Printf(TEXT("Mission check (property: %d)"), (int32)PropertyToCheck);
}

// ---- Реализации CreateRequestPayload для каждого типа ----

UMissionCheckRequestPayload* UMissionCheckCondition_Bool::CreateRequestPayload() const
{
    if (!EventBus) return nullptr;
    UMissionCheckRequestPayload* Payload = EventBus->CreatePayload<UMissionCheckRequestPayload>();
    Payload->DataType = ECheckDataType::Bool;
    Payload->Operator = Operator;
    Payload->StringValue = ExpectedValue ? TEXT("true") : TEXT("false");
    return Payload;
}

UMissionCheckRequestPayload* UMissionCheckCondition_Int::CreateRequestPayload() const
{
    if (!EventBus) return nullptr;
    UMissionCheckRequestPayload* Payload = EventBus->CreatePayload<UMissionCheckRequestPayload>();
    Payload->DataType = ECheckDataType::Int32;
    Payload->Operator = Operator;
    Payload->StringValue = FString::FromInt(ExpectedValue);
    return Payload;
}

UMissionCheckRequestPayload* UMissionCheckCondition_Float::CreateRequestPayload() const
{
    if (!EventBus) return nullptr;
    UMissionCheckRequestPayload* Payload = EventBus->CreatePayload<UMissionCheckRequestPayload>();
    Payload->DataType = ECheckDataType::Float;
    Payload->Operator = Operator;
    Payload->StringValue = FString::SanitizeFloat(ExpectedValue);
    return Payload;
}

UMissionCheckRequestPayload* UMissionCheckCondition_String::CreateRequestPayload() const
{
    if (!EventBus) return nullptr;
    UMissionCheckRequestPayload* Payload = EventBus->CreatePayload<UMissionCheckRequestPayload>();
    Payload->DataType = ECheckDataType::String;
    Payload->Operator = Operator;
    Payload->StringValue = ExpectedValue;
    return Payload;
}