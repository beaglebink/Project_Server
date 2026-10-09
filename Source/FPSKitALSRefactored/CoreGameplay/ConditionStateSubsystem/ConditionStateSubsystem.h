#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ISaveableSubsystem.h"
#include "ConditionStateSubsystem.generated.h"

/**
 * Хранит моменты «когда условие в последний раз стало истинным».
 * Используется UTimeWindowConditionAsset для окон «в течение X после события»
 * и «через X после события».
 *
 * Ключ — стабильная строка, которую задаёт сама обёртка (по умолчанию —
 * путь ассета обёртки). Значения — в игровом времени (UChoreManagerSubsystem
 * ::GetGameTime()), чтобы окна не «тикали», пока игра закрыта.
 */
UCLASS()
class FPSKITALSREFACTORED_API UConditionStateSubsystem
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
    virtual FString GetSaveSubsystemName() const override { return TEXT("ConditionState"); }
    virtual bool GetIsLoadComplete() const override { return bLoadComplete; }

    // Момент последнего подъёма условия. FDateTime::MinValue() — если не зафиксирован.
    FDateTime FindRiseTime(FName Key) const;

    // Зафиксировать/сбросить момент подъёма.
    void SetRiseTime(FName Key, FDateTime Time);
    void ClearRiseTime(FName Key);

private:
    UPROPERTY()
    TMap<FName, FDateTime> RiseTimes;

    bool bLoadComplete = true;
};