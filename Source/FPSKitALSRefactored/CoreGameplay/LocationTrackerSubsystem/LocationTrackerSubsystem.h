#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ISaveableSubsystem.h"

#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"

#include "LocationTrackerSubsystem.generated.h"

class UWorldMapAsset;
class UWorldRegionAsset;
class UStreetAsset;
class UInteriorSetAsset;
class UFloorAsset;

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

    FString GetCurrentLevelPackageName() const;

    static FString NormalizeLevelName(const FString& InPath);

    bool HasEnteredEver(const FString& NormalizedPackageName) const;
    bool HasEnteredInWindow(const FString& NormalizedPackageName, float WindowMinutes) const;
    bool HasLeftAndReturned(const FString& NormalizedPackageName, float WindowMinutes) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker")
    int32 GetEnterCount(const FString& NormalizedPackageName) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker")
    int32 GetLeaveCount(const FString& NormalizedPackageName) const;

    const FLocationVisitState* FindVisitState(const FString& NormalizedPackageName) const;

    // Резолвер иерархии локации в список нормализованных имён пакетов сцен.
    // Используется LocationVisitReset.
    static bool ResolveTargetPackageNames(
        UWorldMapAsset* TargetMap,
        UWorldRegionAsset* TargetRegion,
        UStreetAsset* TargetStreet,
        UInteriorSetAsset* TargetBuilding,
        UFloorAsset* TargetFloor,
        TArray<FString>& OutPackageNames);

private:
    FDateTime GetGameTimeNow() const;
    void HandleLocationEvent(const FOutcomeEventBase& Outcome);

    UPROPERTY()
    TMap<FString, FLocationVisitState> VisitHistory;

    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> LocationEventCondition;

    FOutcomeHandlerHandle LocationEventHandler;

    void HandleVisitReset(const FOutcomeEventBase& Outcome);

    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> LocationResetCondition;

    FOutcomeHandlerHandle LocationResetHandler;

    FString CachedCurrentLevelPackageName;

    bool bSuppressNextLevelLoad = true;
    bool bLoadComplete = true;
};