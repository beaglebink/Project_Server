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
