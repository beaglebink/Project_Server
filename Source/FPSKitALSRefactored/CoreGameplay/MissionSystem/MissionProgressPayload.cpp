#include "MissionProgressPayload.h"

void UMissionProgressPayload::OnBeforePublish(const FCommandSpec* Spec, const UPayloadBatchEntry* Entry)
{
    if (Asset)
    {
        MissionName = Asset->GetMissionId();
    }
}
