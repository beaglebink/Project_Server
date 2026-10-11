// PayloadBatchAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PayloadBatchEntry.h"
#include "PayloadBatchAsset.generated.h"

/**
 * Пакет команд.
 *
 *   Entries — список команд (порядок = порядок исполнения).
 *   ExecuteBatch         — прогнать все включённые.
 *   ExecuteBatchFiltered — прогнать только одну подсистему.
 *   ExecuteEntry         — точечный дебаг.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UPayloadBatchAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Batch")
    TArray<TObjectPtr<UPayloadBatchEntry>> Entries;

    // false → на каждую публикацию делаем DuplicateObject(payload).
    //         Безопасно: подписчики могут хранить payload.
    // true  → публикуется тот же инстанс (быстрее, read-only у подписчиков).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Batch")
    bool bReusePayloadInstances = false;

    // =================== API ===================

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
};