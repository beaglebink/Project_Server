#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PayloadBatchEntry.h"
#include "PayloadBatchAsset.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UPayloadBatchAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // ===================== Обычные команды (активация шага) =====================
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "1. Batch|Activation")
    TArray<TObjectPtr<UPayloadBatchEntry>> Entries;

    // ===================== Команды деактивации =====================
    // Выполняются, когда шаг переходит из активного в неактивный.
    // Могут быть пустыми.
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "1. Batch|Deactivation")
    TArray<TObjectPtr<UPayloadBatchEntry>> DeactivationEntries;

    // ===================== Условие шага =====================
    // Все теги Required должны присутствовать в хранилище.
    // Ни один тег Blocked не должен присутствовать.
    // Если оба массива пусты — шаг активен всегда (пока bEnabled).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. Condition")
    TArray<FGameplayTag> RequiredTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. Condition")
    TArray<FGameplayTag> BlockedTags;

    // Отладочный выключатель: если false — шаг всегда неактивен.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. Condition")
    bool bEnabled = true;

    // ===================== Поведение =====================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3. Behaviour")
    bool bReusePayloadInstances = false;

    // ===================== PrimaryAssetId =====================
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    // ===================== API (вызывается из ScenarioManagerSubsystem) =====================

    // Чистая проверка условия без учёта bEnabled и ActiveSteps.
    bool IsConditionMet(const TSet<FGameplayTag>& TagStorage) const;

    // === Публичное API пакетного запуска (как было) ===
    UFUNCTION(BlueprintCallable, Category = "Batch",
        meta = (WorldContext = "WorldContextObject"))
    void ExecuteBatch(UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Batch",
        meta = (WorldContext = "WorldContextObject"))
    void ExecuteBatchFiltered(UObject* WorldContextObject, EBatchSubsystem OnlySubsystem);

    UFUNCTION(BlueprintCallable, Category = "Batch",
        meta = (WorldContext = "WorldContextObject"))
    void ExecuteEntry(UObject* WorldContextObject, UPayloadBatchEntry* Entry);

private:
    void ExecuteInternal(UObject* WorldContextObject, const EBatchSubsystem* Filter);

    // Общий цикл выполнения произвольного массива entry (для сценария).
    void ExecuteEntriesInternal(UObject* WorldContextObject,
        const TArray<TObjectPtr<UPayloadBatchEntry>>& InEntries);

    // Друзья — ScenarioManagerSubsystem вызывает ExecuteEntriesInternal.
    friend class UScenarioManagerSubsystem;
};