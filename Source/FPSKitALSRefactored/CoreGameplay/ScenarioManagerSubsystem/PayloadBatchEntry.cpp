#include "PayloadBatchEntry.h"
#include "OutcomePayload.h"

TArray<FName> UPayloadBatchEntry::GetCommandOptions() const
{
    return FPayloadBatchRegistry::GetCommandNames(Subsystem);
}

const FCommandSpec* UPayloadBatchEntry::GetSpec() const
{
    return FPayloadBatchRegistry::Find(Subsystem, Command);
}

bool UPayloadBatchEntry::IsValidEntry() const
{
    return bEnabled && Payload != nullptr && GetSpec() != nullptr;
}

#if WITH_EDITOR
void UPayloadBatchEntry::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    Super::PostEditChangeProperty(Event);

    const FName PropName = Event.GetPropertyName();
    if (PropName == GET_MEMBER_NAME_CHECKED(UPayloadBatchEntry, Subsystem))
    {
        SyncCommandForSubsystem();
        RebuildPayload();
    }
    else if (PropName == GET_MEMBER_NAME_CHECKED(UPayloadBatchEntry, Command))
    {
        RebuildPayload();
    }
}

void UPayloadBatchEntry::SyncCommandForSubsystem()
{
    const TArray<FName> Options = FPayloadBatchRegistry::GetCommandNames(Subsystem);
    if (Options.Num() == 0) { Command = NAME_None; return; }
    if (!Options.Contains(Command)) { Command = Options[0]; }
}

void UPayloadBatchEntry::RebuildPayload()
{
    const FCommandSpec* Spec = GetSpec();
    if (!Spec || !Spec->PayloadClass) { Payload = nullptr; return; }
    if (Payload && Payload->GetClass() == Spec->PayloadClass) return;
    Payload = NewObject<UOutcomePayload>(this, Spec->PayloadClass);
}
#endif