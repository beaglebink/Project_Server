#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ISaveableSubsystem.h"

#include "OutcomeEventBase.h"                       // FOutcomeEventBase
#include "OutcomeConditionAsset.h"                  // UOutcomeConditionAsset
#include "../EventBusSystem/EventBusSubsystem.h"    // FOutcomeHandlerHandle, UEventBusSubsystem

#include "LocationTrackerSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FLocationVisitState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "LocationTracker")
    FString LevelPackageName;

    UPROPERTY(BlueprintReadOnly, Category = "LocationTracker")
    FDateTime FirstEnteredAt;

    UPROPERTY(BlueprintReadOnly, Category = "LocationTracker")
    FDateTime LastEnteredAt;

    UPROPERTY(BlueprintReadOnly, Category = "LocationTracker")
    FDateTime LastLeftAt;

    UPROPERTY(BlueprintReadOnly, Category = "LocationTracker")
    int32 EnterCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "LocationTracker")
    int32 LeaveCount = 0;
};

UCLASS()
class FPSKITALSREFACTORED_API ULocationTrackerSubsystem
    : public UGameInstanceSubsystem
    , public ISaveableSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ---- ISaveableSubsystem ----
    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    virtual FString GetSaveSubsystemName() const override { return TEXT("LocationTracker"); }
    virtual bool GetIsLoadComplete() const override { return bLoadComplete; }

    // Приводит путь/имя к «каноническому» виду:
    //   /Game/Content/Maps/Level_X → level_x
    // Убирает .umap, PIE-префиксы (UEDPIE_0_), приводит к lowercase.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker")
    static FString NormalizeLevelName(const FString& InPath);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker")
    FString GetCurrentLevelPackageName() const;

    bool HasEnteredEver(const FString& NormalizedPackageName) const;
    bool HasEnteredInWindow(const FString& NormalizedPackageName, float WindowMinutes) const;
    bool HasLeftAndReturned(const FString& NormalizedPackageName, float WindowMinutes) const;
    int32 GetEnterCount(const FString& NormalizedPackageName) const;
    int32 GetLeaveCount(const FString& NormalizedPackageName) const;
    const FLocationVisitState* FindVisitState(const FString& NormalizedPackageName) const;

private:
    FDateTime GetGameTimeNow() const;
    void HandleLocationEvent(const FOutcomeEventBase& Outcome);

    UPROPERTY()
    TMap<FString, FLocationVisitState> VisitHistory;

    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> LocationEventCondition;

    FOutcomeHandlerHandle LocationEventHandler;

    bool bSuppressNextLevelLoad = true;
    bool bLoadComplete = true;
};