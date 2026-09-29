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
    // ---- State-driven (we check the current storage of the subsystem) ----
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

    // ---- Filter by terminal ----
    // ---- Фильтр по терминалу ----
    // false — games on any terminal are considered.
    // false — учитываются игры на любом терминале.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game")
    bool bUseTerminalFilter = false;

    // Terminal GUID as a 32-character hex string.
    // The designer copies it from the terminal component.
    // Parsing into FGuid — on the fly, when evaluating the condition.
    // GUID терминала в виде 32-символьной hex-строки.
    // Дизайнер копирует его из компонента терминала.
    // Парсинг в FGuid — на лету, при оценке условия.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game",
        meta = (EditCondition = "bUseTerminalFilter", EditConditionHides))
    FString TerminalIdString;

    // ---- Filter by game ----
    // ---- Фильтр по игре ----
    // Empty string — any game.
    // Пустая строка — любая игра.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game")
    FString ActivityId;

    // ---- Status (for ReachedStatus) ----
    // ---- Статус (для ReachedStatus) ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game",
        meta = (EditCondition = "QueryType == ETerminalGameQueryType::ReachedStatus",
            EditConditionHides,
            ValidEnumValues = "Started, GameCompleted, Failed"))
    ETerminalRecordStatus Status = ETerminalRecordStatus::GameCompleted;

    // ---- Stage (for StageFinished / AllStagesCompleted) ----
    // ---- Этап (для StageFinished / AllStagesCompleted) ----
    // StageId — which specific stage is of interest (not used for AllStagesCompleted).
    // StageId — какой именно этап интересует (для AllStagesCompleted не используется).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game",
        meta = (EditCondition = "QueryType == ETerminalGameQueryType::StageFinished",
            EditConditionHides))
    FString StageId;

    // How exactly the stage must be finished (Completed / Failed / Skipped).
    // Only taken into account in StageFinished.
    // Как именно этап должен быть завершён (Completed / Failed / Skipped).
    // Учитывается только в StageFinished.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game",
        meta = (EditCondition = "QueryType == ETerminalGameQueryType::StageFinished",
            EditConditionHides,
            ValidEnumValues = "Completed, Failed, Skipped"))
    ETerminalStageResult StageResult = ETerminalStageResult::Completed;

    // ---- Score (for ScoreReached) ----
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
    // Common filters TerminalId / ActivityId.
    // Общие фильтры TerminalId / ActivityId.
    bool PassesCommonFilters(const FTerminalActivityRecord& Record) const;

    // Checks a single record in state mode (depending on QueryType).
    // Проверка одной записи в state-режиме (в зависимости от QueryType).
    bool EvaluateRecordState(const FTerminalActivityRecord& Record) const;

    // Collects records matching TerminalId/ActivityId.
    // Собирает записи, подходящие под TerminalId/ActivityId.
    TArray<FTerminalActivityRecord> CollectCandidateRecords() const;
};