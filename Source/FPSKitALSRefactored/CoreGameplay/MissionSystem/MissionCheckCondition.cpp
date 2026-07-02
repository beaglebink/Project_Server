// MissionCheckCondition.cpp
#include "MissionCheckCondition.h"
#include "CheckCoordinatorComponent.h"
#include "OutcomeConditionAsset.h"
#include "EventBusSubsystem.h"

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

void UMissionCheckCondition::ExecuteCheck(const FGuid& TransactionId)
{
    if (!EventBus) return;
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    UMissionCheckRequestPayload* Payload = CreateRequestPayload();
    if (!Payload)
    {
        UE_LOG(LogTemp, Error, TEXT("MissionCheck: Failed to create payload for MissionId=%s"), *MissionId.ToString());
        return;
    }
    Payload->TransactionId = TransactionId;
    Payload->MissionId = MissionId;
    Payload->PropertyToCheck = PropertyToCheck;

    FOutcomeEventBase Event;
    Event.OutcomeType = EOutcomeType::Mission;
    Event.OutcomeMission = EOutcomeMission::CheckRequest;
    Event.Payload = Payload;
    EventBus->PublishOutcome(Event);

    UE_LOG(LogTemp, Log, TEXT("MissionCheck: Sent request for MissionId=%s, Property=%d, Txn=%s"),
        *MissionId.ToString(), (int32)PropertyToCheck, *TransactionId.ToString());

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
        UE_LOG(LogTemp, Log, TEXT("MissionCheck: Approved for MissionId=%s"), *MissionId.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("MissionCheck: Rejected for MissionId=%s. Reason: %s"), *MissionId.ToString(), *Resp->Reason);
    }

    OnComplete.ExecuteIfBound(this);
}

FString UMissionCheckCondition::GetDescription() const
{
    return FString::Printf(TEXT("Mission check (ID: %s, property: %d)"), *MissionId.ToString(), (int32)PropertyToCheck);
}

// ---- Остальные методы без изменений, кроме использования FName в логах ----
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