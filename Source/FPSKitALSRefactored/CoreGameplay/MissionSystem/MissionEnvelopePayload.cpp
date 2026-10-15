#include "MissionEnvelopePayload.h"

UMissionEnvelopePayload* UMissionEnvelopePayload::Setup(UMissionAsset* InAsset, EMissionEndReason InEndReason)
{
    if (InAsset)
    {
        MissionId = InAsset->GetMissionId();
    }
    MissionAsset = InAsset;
    EndReason = InEndReason;
    return this;
}

void UMissionEnvelopePayload::OnBeforePublish(const FCommandSpec* Spec, const UPayloadBatchEntry* Entry)
{
    // Сохраняем совместимость с существующим Setup: если MissionAsset задан — выставляем MissionId
    if (MissionAsset)
    {
        MissionId = MissionAsset->GetMissionId();
    }
    // Можно также учитывать Spec/Entry при необходимости
}