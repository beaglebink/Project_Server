// CheckCoordinatorComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EventBusSubsystem.h"
#include "CheckCondition.h"
#include "CheckCoordinatorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSimpleCheckDelegate);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPSKITALSREFACTORED_API UCheckCoordinatorComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCheckCoordinatorComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Check")
    TArray<UCheckCondition*> Conditions;

    UPROPERTY(BlueprintAssignable, Category = "Check")
    FOnSimpleCheckDelegate OnAllApproved;

    UPROPERTY(BlueprintAssignable, Category = "Check")
    FOnSimpleCheckDelegate OnAnyRejected;

    UFUNCTION(BlueprintCallable, Category = "Check")
    void StartCheck();

    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
    float GlobalTimeoutSeconds = 10.0f;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY()
    TObjectPtr<UEventBusSubsystem> EventBus;

    struct FPendingCheck
    {
        FGuid TransactionId;
        int32 TotalConditions = 0;
        int32 CompletedCount = 0;
        //FTimerHandle GlobalTimeoutTimer;
        bool bFinalized = false;
    };
    TMap<FGuid, FPendingCheck> ActiveChecks;

    void FinalizeCheck(const FGuid& TransactionId, bool bSuccess);
    //void OnTimeout(FGuid TransactionId);   // метод для таймера
    void OnConditionComplete(UCheckCondition* Condition);
};