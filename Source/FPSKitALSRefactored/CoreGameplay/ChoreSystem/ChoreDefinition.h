#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ChoreEnums.h"
#include "OutcomeConditionAsset.h"
#include "ChoreDefinition.generated.h"

USTRUCT(BlueprintType)
struct FChoreStageDefinition
{
    GENERATED_BODY()

    // Unique stable stage key.
    // Used in conditions, saves, navigation.
    // Уникальный стабильный ключ стадии.
    // Используется в условиях, сейвах, навигации.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
    FName StageKey;

    // UI name (e.g., "Pick up the package").
    // Имя для UI (например, "Забрать посылку").
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stage")
    FText DisplayName;

};

USTRUCT(BlueprintType)
struct FChoreRewardSet
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Money = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> ItemIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Experience = 0;
};

USTRUCT(BlueprintType)
struct FChorePerformanceMetrics
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CompletionTimeSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Mistakes = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Accuracy = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Quantity = 0;
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // ---- Identity ----
    // ---- Идентификация ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    EChoreFamily Family = EChoreFamily::Miscellaneous;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    EChoreSubtype Subtype = EChoreSubtype::Default;

    // Reference to the asset with mini-game configuration (prefab, level, BP, etc.)
    // Ссылка на ассет с конфигурацией мини-игры (префаб, уровень, BP и т.п.)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
    TSoftObjectPtr<UObject> ChoreSetup;

    // ---- Stages (multi-stage chores) ----
    // Empty array = single-stage chore (one implicit stage).
    // The order of elements corresponds to the indices the mini-game sends
    // in AdvanceStageRequest.StageIndex.
    // ---- Стадии (multi-stage chores) ----
    // Пустой массив = single-stage chore (одна неявная стадия).
    // Порядок элементов соответствует индексам, которые миниигра шлёт
    // в AdvanceStageRequest.StageIndex.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stages")
    TArray<FChoreStageDefinition> Stages;

    // ---- Availability ----
    // Condition under which the task becomes available
    // ---- Доступность ----
    // Условие, при котором задание становится доступным
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Availability")
    TObjectPtr<UOutcomeConditionAsset> AvailabilityCondition;

    // Reactivation condition for repeatable tasks (if RetryBehavior == Conditional)
    // Условие реактивации для повторяемых заданий (если RetryBehavior == Conditional)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Availability")
    TObjectPtr<UOutcomeConditionAsset> ReactivationCondition;

    // ---- Behavior ----
    // ---- Поведение ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    bool bMustStartImmediately = false;

    // Deadline in minutes and seconds (for convenience in the editor)
    // Дедлайн в минутах и секундах (для удобства настройки в редакторе)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior", meta = (ClampMin = 0))
    int32 DeadlineMinutes = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior", meta = (ClampMin = 0))
    int32 DeadlineSeconds = 0;

    // Resulting deadline (calculated automatically from minutes and seconds)
    // The field is read-only in the editor to maintain consistency.
    // Результирующий дедлайн (вычисляется автоматически из минут и секунд)
    // Поле доступно только для чтения в редакторе, чтобы не нарушать консистентность.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Behavior")
    FTimespan Deadline;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    EChoreRetryBehavior RetryBehavior = EChoreRetryBehavior::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    bool bIsRepeatable = false;

    // ---- Behavior on AbandonRequest ----
    // ---- Поведение при AbandonRequest ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    EChoreAbandonBehavior AbandonBehavior = EChoreAbandonBehavior::Fail;

    // ---- Rewards ----
    // ---- Награды ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards")
    FChoreRewardSet Rewards;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards")
    bool bShowRewardBeforeAccept = false;

    // ---- Methods ----
    // ---- Методы ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Identity")
    FName GetChoreId() const { return DisplayName.IsEmpty() ? GetFName() : FName(*DisplayName.ToString()); }

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("Chore", GetFName());
    }

    // Update Deadline from the current DeadlineMinutes and DeadlineSeconds values
    // Обновить Deadline из текущих значений DeadlineMinutes и DeadlineSeconds
    void UpdateDeadlineFromMinutesSeconds();

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
    virtual void PostLoad() override;
};