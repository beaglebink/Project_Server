// PayloadBatchAsset.cpp
#include "PayloadBatchAsset.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"       // ← нужен для GetGameInstance()->GetSubsystem
#include "TimerManager.h"
#include "EventBusSubsystem.h"
#include "OutcomeEventBase.h"

void UPayloadBatchAsset::ExecuteEntry(UObject* WorldContextObject, UPayloadBatchEntry* Entry)
{
    if (!WorldContextObject || !Entry || !Entry->IsValidEntry())
        return;

    UWorld* World = WorldContextObject->GetWorld();
    if (!World) return;

    UGameInstance* GI = World->GetGameInstance();
    if (!GI) return;

    // UEventBusSubsystem — GameInstanceSubsystem. Берём из GI, НЕ из World.
    UEventBusSubsystem* Bus = GI->GetSubsystem<UEventBusSubsystem>();
    if (!Bus) return;

    const FCommandSpec* Spec = Entry->GetSpec();
    if (!Spec) return;

    // 1) Payload.
    UOutcomePayload* ToPublish = Entry->Payload;
    if (!bReusePayloadInstances)
    {
        ToPublish = DuplicateObject<UOutcomePayload>(Entry->Payload, GI);
        if (!ToPublish) return;
    }

    // 2) Собираем FOutcomeEventBase и применяем route (фильтры кладутся
    //    в СТРУКТУРУ события, а не в payload).
    FOutcomeEventBase Event;
    FPayloadBatchRegistry::ApplyRouteToEvent(Spec->Route, Event);
    Event.Payload = ToPublish;

    // 3) Публикуем.
    Bus->PublishOutcome(Event);
}

void UPayloadBatchAsset::ExecuteInternal(UObject* WorldContextObject, const EBatchSubsystem* Filter)
{
    if (!WorldContextObject) return;
    UWorld* World = WorldContextObject->GetWorld();
    if (!World) return;

    for (UPayloadBatchEntry* Entry : Entries)
    {
        if (!Entry || !Entry->IsValidEntry()) continue;
        if (Filter && Entry->Subsystem != *Filter) continue;

        if (Entry->DelaySeconds <= 0.0f)
        {
            ExecuteEntry(WorldContextObject, Entry);
            continue;
        }

        TWeakObjectPtr<UPayloadBatchAsset> WeakThis(this);
        TWeakObjectPtr<UPayloadBatchEntry> WeakEntry(Entry);
        TWeakObjectPtr<UObject>            WeakCtx(WorldContextObject);

        FTimerDelegate Del = FTimerDelegate::CreateWeakLambda(this,
            [WeakThis, WeakEntry, WeakCtx]()
            {
                if (!WeakThis.IsValid() || !WeakEntry.IsValid() || !WeakCtx.IsValid())
                    return;
                WeakThis->ExecuteEntry(WeakCtx.Get(), WeakEntry.Get());
            });

        FTimerHandle Handle;
        World->GetTimerManager().SetTimer(Handle, Del, Entry->DelaySeconds, /*bLoop=*/false);
    }
}

void UPayloadBatchAsset::ExecuteBatch(UObject* WorldContextObject)
{
    ExecuteInternal(WorldContextObject, nullptr);
}

void UPayloadBatchAsset::ExecuteBatchFiltered(UObject* WorldContextObject, EBatchSubsystem OnlySubsystem)
{
    ExecuteInternal(WorldContextObject, &OnlySubsystem);
}