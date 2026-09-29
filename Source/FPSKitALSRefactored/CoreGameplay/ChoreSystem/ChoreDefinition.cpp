#include "ChoreDefinition.h"

void UChoreDefinition::UpdateDeadlineFromMinutesSeconds()
{
    Deadline = FTimespan::FromMinutes(DeadlineMinutes) + FTimespan::FromSeconds(DeadlineSeconds);
}

#if WITH_EDITOR
void UChoreDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // If minutes or seconds changed – recalculate the deadline
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

    // On load, always recalculate Deadline from minutes and seconds
    // to guarantee it is up to date, even if the data was changed outside the editor.
    // При загрузке всегда пересчитываем Deadline из минут и секунд,
    // чтобы гарантировать актуальность, даже если данные были изменены вне редактора.
    UpdateDeadlineFromMinutesSeconds();
}