// ChoreDefinition.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ChoreEnums.h"
#include "OutcomeConditionAsset.h"
#include "ChoreDefinition.generated.h"

USTRUCT(BlueprintType)
struct FChoreRewardSet
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Money = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> ItemIds;   // или использовать FPrimaryAssetId

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

    // Можно добавлять другие метрики по необходимости
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // ---- Идентификация ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    EChoreFamily Family = EChoreFamily::Miscellaneous;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    EChoreSubtype Subtype = EChoreSubtype::Default;

    // Ссылка на ассет с конфигурацией мини-игры (префаб, уровень, BP и т.п.)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Setup")
    TSoftObjectPtr<UObject> ChoreSetup;

    // ---- Доступность ----
    // Условие, при котором задание становится доступным
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Availability")
    TObjectPtr<UOutcomeConditionAsset> AvailabilityCondition;

    // Условие реактивации для повторяемых заданий (если RetryBehavior == Conditional)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Availability")
    TObjectPtr<UOutcomeConditionAsset> ReactivationCondition;

    // ---- Поведение ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    bool bMustStartImmediately = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    FTimespan Deadline; // нулевой = без дедлайна

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    EChoreRetryBehavior RetryBehavior = EChoreRetryBehavior::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    bool bIsRepeatable = false;

    // ---- Награды ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards")
    FChoreRewardSet Rewards;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards")
    bool bShowRewardBeforeAccept = false;

    // ---- Методы ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Identity")
    FName GetChoreId() const { return DisplayName.IsEmpty() ? GetFName() : FName(*DisplayName.ToString()); }

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("Chore", GetFName());
    }
};