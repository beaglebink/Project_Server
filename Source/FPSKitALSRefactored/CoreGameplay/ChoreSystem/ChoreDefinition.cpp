#include "ChoreDefinition.h"

void UChoreDefinition::UpdateDeadlineFromMinutesSeconds()
{
    Deadline = FTimespan::FromMinutes(DeadlineMinutes) + FTimespan::FromSeconds(DeadlineSeconds);
}

#if WITH_EDITOR
void UChoreDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Если изменились минуты или секунды – пересчитываем дедлайн
    if (PropertyChangedEvent.Property)
    {
        const FName PropName = PropertyChangedEvent.Property->GetFName();
        if (PropName == GET_MEMBER_NAME_CHECKED(UChoreDefinition, DeadlineMinutes) ||
            PropName == GET_MEMBER_NAME_CHECKED(UChoreDefinition, DeadlineSeconds))
        {
            UpdateDeadlineFromMinutesSeconds();
        }
    }
}
#endif

void UChoreDefinition::PostLoad()
{
    Super::PostLoad();

    // При загрузке всегда пересчитываем Deadline из минут и секунд,
    // чтобы гарантировать актуальность, даже если данные были изменены вне редактора.
    UpdateDeadlineFromMinutesSeconds();
}