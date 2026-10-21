#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "ISaveableSubsystem.h"
#include "OutcomeEventBase.h"
#include "EventBusSubsystem.h"
#include "ScenarioManagerSubsystem.generated.h"

class UPayloadBatchAsset;
class UPayloadBatchEntry;
class UOutcomeConditionAsset;
struct FCommandSpec;
struct FSubsystemSaveData;

UCLASS()
class FPSKITALSREFACTORED_API UScenarioManagerSubsystem
    : public UGameInstanceSubsystem
    , public ISaveableSubsystem
{
    GENERATED_BODY()

public:
    // ---- UGameInstanceSubsystem ----
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ---- ISaveableSubsystem ----
    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    virtual FString GetSaveSubsystemName() const override { return TEXT("ScenarioManager"); }
    virtual bool GetIsLoadComplete() const override { return bLoadComplete; }

    // =================== Public API ===================

    UFUNCTION(BlueprintCallable, Category = "Scenario Manager")
    void ChangeTags(const TArray<FGameplayTag>& TagsToAdd,
        const TArray<FGameplayTag>& TagsToRemove);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenario Manager|Query")
    bool HasTag(FGameplayTag Tag) const { return Tag.IsValid() && TagStorage.Contains(Tag); }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenario Manager|Query")
    TArray<FGameplayTag> GetAllTags() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenario Manager|Debug")
    TArray<FName> GetActiveStepNames() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenario Manager|Debug")
    FString GetDebugStateString() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scenario Manager|Query")
    bool IsStepActive(const UPayloadBatchAsset* Step) const;

protected:
    // ---- Инициализация ----
    void LoadAllSteps();
    void SubscribeToEventBus();
    void SubscribeToInitialLevelLoaded();

    // ---- Переоценка ----
    void ReevaluateSteps();

    // ---- Выполнение команд ----
    void ExecuteStepActivation(UPayloadBatchAsset* Step);
    void ExecuteStepDeactivation(UPayloadBatchAsset* Step);

    // ---- EventBus handlers ----
    void HandleOutcomeWithTags(const FOutcomeEventBase& Outcome);
    void OnInitialLevelLoaded(const FOutcomeEventBase& Outcome);

    // Точка входа переоценки при каждой загрузке уровня.
    // Вызывается из OnInitialLevelLoaded, fallback-таймера
    // и из ApplySaveData (если LevelLoaded пришёл до сейва).
    void ReevaluateOnLevelLoaded(const TCHAR* Reason);

    // Переприменяет stateful-эффекты активного шага при загрузке уровня.
    void ExecuteStepReapply(UPayloadBatchAsset* Step);

    // Возвращает true, если entry надо переприменить при загрузке.
    static bool ShouldReapplyOnLoad(UPayloadBatchEntry* Entry);

private:
    UPROPERTY()
    TMap<FPrimaryAssetId, TObjectPtr<UPayloadBatchAsset>> Steps;

    UPROPERTY()
    TSet<FGameplayTag> TagStorage;

    UPROPERTY()
    TSet<FPrimaryAssetId> ActiveStepIds;

    UPROPERTY()
    bool bLoadComplete = true;

    bool bIsReevaluating = false;
    bool bNeedsReevaluate = false;
    static constexpr int32 MaxReevaluateIterations = 32;

    // ---- EventBus: теги ----
    FOutcomeHandlerHandle TagChangeHandler;
    UPROPERTY() TObjectPtr<UOutcomeConditionAsset> TagChangeCondition;

    // ---- EventBus: LevelLoaded ----
    // Хендлер живёт всю игру; при каждом LevelLoaded вызывается
    // ReevaluateOnLevelLoaded. Не отписываемся после первого вызова.
    FOutcomeHandlerHandle InitialLevelLoadedHandler;
    UPROPERTY() TObjectPtr<UOutcomeConditionAsset> InitialLevelLoadedCondition;

    FTimerHandle InitialReevaluateFallbackTimer;
    static constexpr float InitialReevaluateFallbackDelay = 1.0f;  // сек
};