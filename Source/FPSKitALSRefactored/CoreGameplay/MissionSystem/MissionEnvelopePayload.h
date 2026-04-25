#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "MissionEnvelopeTypes.h"
#include "MissionEnvelopePayload.generated.h"

// ??? UMissionEnvelopePayload ???????????????????????????????????????????????????
// Payload для событий регистрации / снятия Envelope миссии через EventBus.
// OutcomeType = Mission, OutcomeMission = MissionActivated / MissionCompleted / etc.
//
// Blueprint-использование:
//   1. Создать через EventBus->CreatePayload(UMissionEnvelopePayload)
//   2. Заполнить поля через Setup()
//   3. PublishOutcome с OutcomeType=Mission и OutcomeMission=MissionActivated
//
// MissionSubsystem подписан и при получении вызывает CreateMission + ActivateMission.
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UMissionEnvelopePayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    // Стабильный FName-идентификатор миссии (имя ассета UMissionAsset)
    UPROPERTY(BlueprintReadWrite, Category = "MissionEnvelope")
    FName MissionId;

    // Ссылка на ассет миссии (заполняется до публикации события)
    UPROPERTY(BlueprintReadWrite, Category = "MissionEnvelope")
    TObjectPtr<class UMissionAsset> MissionAsset;

    // Причина завершения (заполняется при MissionCompleted/Failed/Abandoned)
    UPROPERTY(BlueprintReadWrite, Category = "MissionEnvelope")
    EMissionEndReason EndReason = EMissionEndReason::None;

    // Удобный Setup для C++ и Blueprint
    UFUNCTION(BlueprintCallable, Category = "MissionEnvelope")
    UMissionEnvelopePayload* Setup(FName InMissionId, UMissionAsset* InAsset,
                                    EMissionEndReason InEndReason = EMissionEndReason::None)
    {
        MissionId   = InMissionId;
        MissionAsset = InAsset;
        EndReason   = InEndReason;
        return this;
    }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionEnvelope")
    FName GetMissionId() const { return MissionId; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionEnvelope")
    EMissionEndReason GetEndReason() const { return EndReason; }
};
