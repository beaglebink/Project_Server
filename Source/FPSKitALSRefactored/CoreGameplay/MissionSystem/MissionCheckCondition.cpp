#include "MissionCheckCondition.h"
#include "CheckCoordinatorComponent.h"
#include "OutcomeConditionAsset.h"
#include "EventBusSubsystem.h"
#include "MissionSubsystem.h"
#include "Engine/GameInstance.h"

// Base class
// Базовый класс

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
    UGameInstance* GI = EventBus->GetGameInstance();
    if (!GI) return NAME_None;
    UMissionSubsystem* MissionSub = GI->GetSubsystem<UMissionSubsystem>();
    if (!MissionSub) return NAME_None;
    return MissionSub->GetActiveMissionId();
}

void UMissionCheckCondition::ExecuteCheck(const FGuid& TransactionId)
{
    if (!EventBus) return;

    // Get the active mission (may be NAME_None)
    // Получаем активную миссию (может быть NAME_None)
    FName ActiveMissionId = GetActiveMissionId();

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
    Payload->MissionId = ActiveMissionId; // may be NAME_None
    Payload->PropertyToCheck = GetCheckedProperty();

    FOutcomeEventBase Event;
    Event.OutcomeType = EOutcomeType::Mission;
    Event.OutcomeMission = EOutcomeMission::CheckRequest;
    Event.Payload = Payload;
    EventBus->PublishOutcome(Event);

    UE_LOG(LogTemp, Log, TEXT("MissionCheck: Sent request for MissionId=%s, Property=%d, Txn=%s"),
        *ActiveMissionId.ToString(), (int32)GetCheckedProperty(), *TransactionId.ToString());

    //StartTimeoutTimer(TransactionId);
}

void UMissionCheckCondition::OnCheckResponse(const FOutcomeEventBase& Event)
{
    if (bCompleted) return;

    UCheckResponsePayload* Resp = Cast<UCheckResponsePayload>(Event.Payload);
    if (!Resp || Resp->TransactionId != CurrentTransactionId) return;
    if (Event.OutcomeMission != EOutcomeMission::CheckResponse) return;

    //ClearTimeoutTimer();
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
    const TCHAR* PropertyName = TEXT("Unknown");
    switch (GetCheckedProperty())
    {
    case EMissionCheckProperty::IsActive:    PropertyName = TEXT("Active"); break;
    case EMissionCheckProperty::CurrentStep: PropertyName = TEXT("Step"); break;
    case EMissionCheckProperty::Name:        PropertyName = TEXT("Name"); break;
    }
    return FString::Printf(TEXT("Mission check (%s)"), PropertyName);
}

// Implementations of CreateRequestPayload
// Реализации CreateRequestPayload

UMissionCheckRequestPayload* UMissionCheckCondition_IsActive::CreateRequestPayload() const
{
    if (!EventBus) return nullptr;
    UMissionCheckRequestPayload* Payload = EventBus->CreatePayload<UMissionCheckRequestPayload>();
    Payload->DataType = ECheckDataType::Bool;
    Payload->Operator = Operator;
    Payload->StringValue = ExpectedValue ? TEXT("true") : TEXT("false");
    return Payload;
}

UMissionCheckRequestPayload* UMissionCheckCondition_Step::CreateRequestPayload() const
{
    if (!EventBus) return nullptr;
    UMissionCheckRequestPayload* Payload = EventBus->CreatePayload<UMissionCheckRequestPayload>();
    Payload->DataType = ECheckDataType::Int32;
    Payload->Operator = Operator;
    Payload->StringValue = FString::FromInt(ExpectedValue);
    return Payload;
}

UMissionCheckRequestPayload* UMissionCheckCondition_Name::CreateRequestPayload() const
{
    if (!EventBus) return nullptr;
    UMissionCheckRequestPayload* Payload = EventBus->CreatePayload<UMissionCheckRequestPayload>();
    Payload->DataType = ECheckDataType::String;
    Payload->Operator = Operator;
    Payload->StringValue = ExpectedValue;
    return Payload;
}