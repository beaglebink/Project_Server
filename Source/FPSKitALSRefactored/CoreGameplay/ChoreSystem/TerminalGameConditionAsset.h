#pragma once

#include "CoreMinimal.h"
#include "OutcomeConditionAsset.h"
#include "CheckRequestPayload.h"                 // ECheckCompareOp
#include "../TerminalSystem/TerminalSubsystem.h" // ETerminalRecordStatus, ETerminalStageResult, FTerminalActivityRecord
#include "Outcome.h"                             // EOutcomeType, EOutcomeTerminal
#include "TerminalGameConditionAsset.generated.h"

UENUM(BlueprintType)
enum class ETerminalGameQueryType : uint8
{
    // ---- State-driven (проверяем текущее хранилище подсистемы) ----
    HasGameRecord       UMETA(DisplayName = "Has Game Record (state)"),
    ReachedStatus       UMETA(DisplayName = "Reached Status (state)"),
    StageFinished       UMETA(DisplayName = "Stage Finished (state)"),
    AllStagesCompleted  UMETA(DisplayName = "All Stages Completed (state)"),
    ScoreReached        UMETA(DisplayName = "Score Reached (state)")
};

UCLASS(BlueprintType, ShowCategories = ("Terminal Game", "4 - Debug"))
class FPSKITALSREFACTORED_API UTerminalGameConditionAsset : public UOutcomeConditionAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game")
    ETerminalGameQueryType QueryType = ETerminalGameQueryType::HasGameRecord;

    // ---- Фильтр по терминалу ----
    // false — учитываются игры на любом терминале.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game")
    bool bUseTerminalFilter = false;

    // GUID терминала в виде 32-символьной hex-строки.
    // Дизайнер копирует его из компонента терминала.
    // Парсинг в FGuid — на лету, при оценке условия.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game",
        meta = (EditCondition = "bUseTerminalFilter", EditConditionHides))
    FString TerminalIdString;

    // ---- Фильтр по игре ----
    // Пустая строка — любая игра.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game")
    FString ActivityId;

    // ---- Статус (для ReachedStatus) ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game",
        meta = (EditCondition = "QueryType == ETerminalGameQueryType::ReachedStatus",
            EditConditionHides))
    ETerminalRecordStatus Status = ETerminalRecordStatus::GameCompleted;

    // ---- Этап (для StageFinished / AllStagesCompleted) ----
    // StageId — какой именно этап интересует (для AllStagesCompleted не используется).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game",
        meta = (EditCondition = "QueryType == ETerminalGameQueryType::StageFinished",
            EditConditionHides))
    FString StageId;

    // Как именно этап должен быть завершён (Completed / Failed / Skipped).
    // Учитывается только в StageFinished.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game",
        meta = (EditCondition = "QueryType == ETerminalGameQueryType::StageFinished",
            EditConditionHides))
    ETerminalStageResult StageResult = ETerminalStageResult::Completed;

    // ---- Счёт (для ScoreReached) ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game",
        meta = (EditCondition = "QueryType == ETerminalGameQueryType::ScoreReached",
            EditConditionHides))
    int32 ScoreThreshold = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game",
        meta = (EditCondition = "QueryType == ETerminalGameQueryType::ScoreReached",
            EditConditionHides))
    ECheckCompareOp ScoreCompareOp = ECheckCompareOp::GreaterOrEqual;

    virtual void CompileCondition() override;
    bool EvaluateCondition(const FOutcomeEventBase& Outcome) const;

private:
    // Общие фильтры TerminalId / ActivityId.
    bool PassesCommonFilters(const FTerminalActivityRecord& Record) const;

    // Проверка одной записи в state-режиме (в зависимости от QueryType).
    bool EvaluateRecordState(const FTerminalActivityRecord& Record) const;

    // Собирает записи, подходящие под TerminalId/ActivityId.
    TArray<FTerminalActivityRecord> CollectCandidateRecords() const;
};