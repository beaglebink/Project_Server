#pragma once

#include "CoreMinimal.h"
#include "OutcomeConditionAsset.h"
#include "CheckRequestPayload.h"                 // ECheckCompareOp
#include "../TerminalSystem/TerminalSubsystem.h" // ETerminalRecordStatus, FTerminalActivityRecord
#include "Outcome.h"                             // EOutcomeType, EOutcomeTerminal
#include "TerminalGameConditionAsset.generated.h"

UENUM(BlueprintType)
enum class ETerminalGameQueryType : uint8
{
    // ---- State-driven (проверяем текущее хранилище подсистемы) ----
    HasGameRecord       UMETA(DisplayName = "Has Game Record (state)"),
    ReachedStatus       UMETA(DisplayName = "Reached Status (state)"),
    StageCompleted      UMETA(DisplayName = "Stage Completed (state)"),
    AllStagesCompleted  UMETA(DisplayName = "All Stages Completed (state)"),
    ScoreReached        UMETA(DisplayName = "Score Reached (state)"),

    // ---- Event-driven (реагируем на события подсистемы) ----
    OnGameStatusChanged UMETA(DisplayName = "On Game Status Changed (event)"),
    OnStageCompleted    UMETA(DisplayName = "On Stage Completed (event)"),
    OnGameRecordRemoved UMETA(DisplayName = "On Game Record Removed (event)")
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game",
        meta = (EditCondition = "bUseTerminalFilter", EditConditionHides))
    FGuid TerminalId;

    // ---- Фильтр по игре ----
    // Пустая строка — любая игра.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game")
    FString ActivityId;

    // ---- Статус (для ReachedStatus / OnGameStatusChanged) ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game",
        meta = (EditCondition =
            "QueryType == ETerminalGameQueryType::ReachedStatus || QueryType == ETerminalGameQueryType::OnGameStatusChanged",
            EditConditionHides))
    ETerminalRecordStatus Status = ETerminalRecordStatus::GameCompleted;

    // ---- Этап (для StageCompleted / OnStageCompleted) ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal Game",
        meta = (EditCondition =
            "QueryType == ETerminalGameQueryType::StageCompleted || QueryType == ETerminalGameQueryType::OnStageCompleted",
            EditConditionHides))
    FString StageId;

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
    // Общие фильтры TerminalId / ActivityId — применяются и к state-, и к event-driven.
    bool PassesCommonFilters(const FTerminalActivityRecord& Record) const;

    // Проверка одной записи в state-режиме (в зависимости от QueryType).
    bool EvaluateRecordState(const FTerminalActivityRecord& Record) const;

    // Собирает записи, подходящие под TerminalId/ActivityId.
    TArray<FTerminalActivityRecord> CollectCandidateRecords() const;

    // Разбор события GameRecordUpdated / GameRecordRemoved.
    bool MatchGameEvent(const FOutcomeEventBase& Outcome,
        EOutcomeTerminal ExpectedEvent,
        const TFunctionRef<bool(const FTerminalActivityRecord&)>& Predicate) const;
};