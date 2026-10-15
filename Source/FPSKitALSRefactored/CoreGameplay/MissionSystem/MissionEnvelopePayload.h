#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "MissionEnvelopeTypes.h"
#include "MissionAsset.h"
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
    //UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MissionEnvelope")
    FName MissionId;

    // Ссылка на ассет миссии (заполняется до публикации события)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionEnvelope")
    TObjectPtr<class UMissionAsset> MissionAsset;

    // Причина завершения (заполняется при MissionCompleted/Failed/Abandoned)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionEnvelope")
    EMissionEndReason EndReason = EMissionEndReason::None;

    // Удобный Setup для C++ и Blueprint
    UFUNCTION(BlueprintCallable, Category = "MissionEnvelope")
    UMissionEnvelopePayload* Setup(/*FName InMissionId, */UMissionAsset* InAsset, EMissionEndReason InEndReason = EMissionEndReason::Completed);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionEnvelope")
    FName GetMissionId() const { return MissionAsset->GetMissionId(); }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionEnvelope")
    EMissionEndReason GetEndReason() const { return EndReason; }
};
