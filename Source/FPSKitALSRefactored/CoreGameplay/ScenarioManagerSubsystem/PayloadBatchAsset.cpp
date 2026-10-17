// PayloadBatchAsset.cpp
#include "PayloadBatchAsset.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"

#include "EventBusSubsystem.h"
#include "OutcomeEventBase.h"
#include "PayloadBatchEntry.h"
#include "PayloadBatchRegistry.h"
#include "OutcomePayload.h"

// =============================================================================
// PrimaryAssetId
// =============================================================================

FPrimaryAssetId UPayloadBatchAsset::GetPrimaryAssetId() const
{
    // Тип "ScenarioStep" должен совпадать с PrimaryAssetTypesToScan в DefaultGame.ini.
    return FPrimaryAssetId(TEXT("ScenarioStep"), GetFName());
}

// =============================================================================
// Условие шага сценария
// =============================================================================

bool UPayloadBatchAsset::IsConditionMet(const TSet<FGameplayTag>& TagStorage) const
{
    for (const FGameplayTag& Tag : RequiredTags)
    {
        if (Tag.IsValid() && !TagStorage.Contains(Tag))
            return false;
    }
    for (const FGameplayTag& Tag : BlockedTags)
    {
        if (Tag.IsValid() && TagStorage.Contains(Tag))
            return false;
    }
    return true;
}

// =============================================================================
// Внутренний цикл выполнения произвольного массива entry
// =============================================================================

void UPayloadBatchAsset::ExecuteEntriesInternal(
    UObject* WorldContextObject,
    const TArray<TObjectPtr<UPayloadBatchEntry>>& InEntries)
{
    if (!WorldContextObject) return;
    UWorld* World = WorldContextObject->GetWorld();
    if (!World) return;

    for (UPayloadBatchEntry* Entry : InEntries)
    {
        if (!Entry || !Entry->IsValidEntry()) continue;
        ExecuteEntry(WorldContextObject, Entry);
    }
}

// =============================================================================
// Выполнение одной команды
// =============================================================================

void UPayloadBatchAsset::ExecuteEntry(UObject* WorldContextObject, UPayloadBatchEntry* Entry)
{
    if (!WorldContextObject || !Entry || !Entry->IsValidEntry())
        return;

    UWorld* World = WorldContextObject->GetWorld();
    if (!World) return;

    UGameInstance* GI = World->GetGameInstance();
    if (!GI) return;

    // UEventBusSubsystem — GameInstanceSubsystem. Берём из GI, не из World.
    UEventBusSubsystem* Bus = GI->GetSubsystem<UEventBusSubsystem>();
    if (!Bus) return;

    const FCommandSpec* Spec = Entry->GetSpec();
    if (!Spec) return;

    // 1) Payload (duplicate, чтобы подписчики могли безопасно хранить ссылку).
    UOutcomePayload* ToPublish = Entry->Payload;
    if (!bReusePayloadInstances)
    {
        ToPublish = DuplicateObject<UOutcomePayload>(Entry->Payload, GI);
        if (!ToPublish) return;
    }

    ToPublish->OnBeforePublish(Spec, Entry);

    // 2) FOutcomeEventBase + route из реестра.
    FOutcomeEventBase Event;
    FPayloadBatchRegistry::ApplyRouteToEvent(Spec->Route, Event);
    Event.Payload = ToPublish;

    // 3) NEW: отметка шага-источника — для отката отложенных эффектов.
    Event.SourceStepId = GetPrimaryAssetId();

    // 4) Публикуем.
    Bus->PublishOutcome(Event);
}

// =============================================================================
// Пакетный запуск (активация/деактивация шага, отладочные вызовы)
// =============================================================================

void UPayloadBatchAsset::ExecuteBatch(UObject* WorldContextObject)
{
    ExecuteInternal(WorldContextObject, nullptr);
}

void UPayloadBatchAsset::ExecuteBatchFiltered(UObject* WorldContextObject, EBatchSubsystem OnlySubsystem)
{
    ExecuteInternal(WorldContextObject, &OnlySubsystem);
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

        // Отложенный запуск. Слабые указатели — безопасно, если мир/ассет/entry умрут.
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