// CheckCondition.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EventBusSubsystem.h"
#include "CheckRequestPayload.h"
#include "CheckResponsePayload.h"
#include "OutcomeConditionAsset.h"
#include "CheckCondition.generated.h"

class UCheckCoordinatorComponent;

UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class FPSKITALSREFACTORED_API UCheckCondition : public UObject
{
    GENERATED_BODY()
public:
    virtual void Initialize(UCheckCoordinatorComponent* InCoordinator, UEventBusSubsystem* InEventBus);
    virtual void ExecuteCheck(const FGuid& TransactionId) PURE_VIRTUAL(UCheckCondition::ExecuteCheck, );
    virtual void OnCheckResponse(const FOutcomeEventBase& Event) PURE_VIRTUAL(UCheckCondition::OnCheckResponse, );
    virtual bool IsApproved() const PURE_VIRTUAL(UCheckCondition::IsApproved, return false;);
    virtual bool IsCompleted() const PURE_VIRTUAL(UCheckCondition::IsCompleted, return false;);
    virtual FString GetDescription() const PURE_VIRTUAL(UCheckCondition::GetDescription, return TEXT(""); );
    virtual void Reset();

    DECLARE_DELEGATE_OneParam(FOnConditionComplete, UCheckCondition*);
    FOnConditionComplete OnComplete;

    FGuid GetCurrentTransactionId() const { return CurrentTransactionId; }
    float TimeoutSeconds = 3.0f;

    // Логирование
    static int32 CurrentDepth;
    static bool bEnableVerboseLogging;
    void LogVerbose(const FString& Message, bool bIsStart = true);

    UPROPERTY(/*VisibleAnywhere, Category = "Debug"*/)
    FString DebugDescription;

    UPROPERTY(BlueprintReadWrite, Category = "Interior")
    FGuid ItemId;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
    UPROPERTY()
    TObjectPtr<UCheckCoordinatorComponent> Coordinator;

    UPROPERTY()
    TObjectPtr<UEventBusSubsystem> EventBus;

    FGuid CurrentTransactionId;

    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const PURE_VIRTUAL(UCheckCondition::CreateSubscriptionCondition, return nullptr;);
    UWorld* GetWorld() const;

    bool bCompleted = false;
    bool bApproved = false;
};