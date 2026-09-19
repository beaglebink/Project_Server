#include "WorldStateSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "ChangingLocationAvailabilityPayload.h"
#include "../InteriorInstanceSystem/FloorAssignmentComponent.h"
#include "../SaveGame/GameSaveSubsystem.h"
#include "EngineUtils.h"
#include "JsonObjectConverter.h"
#include "WorldStateRecordPayload.h"
#include "WorldStateRecordRemovePayload.h"
#include "WorldStateFactChangedPayload.h"
#include "LevelLoadedPayload.h"
#include <InteriorSubsystem.h>

void UWorldStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    Collection.InitializeDependency<UInteriorSubsystem>();

    // Форсируем инициализацию системы сохранения ДО нас.
    Collection.InitializeDependency<UGameSaveSubsystem>();

    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->RegisterSaveableSubsystem(this);
    }

    SubscribeAllWorldStateEvents();
}

void UWorldStateSubsystem::SubscribeAllWorldStateEvents()
{
    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus)
        return;

    // ---- Установка записи ----
    WorldStateAddRecordCondition = CreateSimpleWorldStateCondition(EOutcomeWorldState::WorldStateAddRecord);
    WorldStateRecordHandle = EventBus->RegisterHandler(
        WorldStateAddRecordCondition,
        FOutcomeHandlerDelegate::CreateUObject(this, &UWorldStateSubsystem::HandleSetWorldStateRecord));

    // ---- Удаление записи ----
    WorldStateRemoveRecordCondition = CreateSimpleWorldStateCondition(EOutcomeWorldState::WorldStateRemoveRecord);
    WorldStateRecordRemoveHandle = EventBus->RegisterHandler(
        WorldStateRemoveRecordCondition,
        FOutcomeHandlerDelegate::CreateUObject(this, &UWorldStateSubsystem::HandleRemoveWorldStateRecord));

    // ---- Загрузка уровня (InteriorSubsystem публикует после восстановления снапшотов) ----
    if (!LevelLoadedHandle.IsValid())
    {
        LevelLoadedConditionAsset = NewObject<UOutcomeConditionAsset>(this);
        LevelLoadedConditionAsset->OperatorType = EConditionOperator::Composite;
        LevelLoadedConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
        LevelLoadedConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
        LevelLoadedConditionAsset->FilterRow.InteriorType = EOutcomeInterior::LevelLoaded;
        LevelLoadedConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
        LevelLoadedConditionAsset->CompileCondition();

        if (LevelLoadedConditionAsset->GetCondition().IsValid())
        {
            LevelLoadedHandle = EventBus->RegisterHandler(
                LevelLoadedConditionAsset,
                FOutcomeHandlerDelegate::CreateUObject(this, &UWorldStateSubsystem::HandleLevelLoaded));
        }
    }
}

void UWorldStateSubsystem::Deinitialize()
{
    UnsubscribeAll();

    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->UnregisterSaveableSubsystem(this);
    }

    WorldStateRecords.Empty();
    SubscribedWorld = nullptr;

    Super::Deinitialize();
}

UOutcomeConditionAsset* UWorldStateSubsystem::CreateSimpleWorldStateCondition(EOutcomeWorldState WorldStateType)
{
    UOutcomeConditionAsset* Asset = NewObject<UOutcomeConditionAsset>(this);
    Asset->OperatorType = EConditionOperator::Composite;
    Asset->FilterRow.OutcomeType = EOutcomeType::WorldState;
    Asset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    Asset->FilterRow.WorldStateType = WorldStateType;
    Asset->FilterRow.WorldStateComparison = EConditionComparison::Equals;
    Asset->CompileCondition();
    return Asset;
}

void UWorldStateSubsystem::UnsubscribeAll()
{
    if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        auto Unreg = [&](FOutcomeHandlerHandle& Handle)
            {
                if (Handle.IsValid())
                {
                    EventBus->UnregisterHandler(Handle);
                    Handle.Invalidate();
                }
            };
        Unreg(WorldStateRecordHandle);
        Unreg(WorldStateRecordRemoveHandle);
        Unreg(LevelLoadedHandle);
    }

    LevelLoadedConditionAsset = nullptr;

    UnsubscribeFromActorSpawned();
}

// ─────────────────────────────────────────────────────────────────────────────
// WORLD STATE RECORDS — ПРИВАТНЫЕ МЕТОДЫ ИЗМЕНЕНИЯ
// ─────────────────────────────────────────────────────────────────────────────

void UWorldStateSubsystem::SetWorldStateRecord(const FWorldStateRecord& Record)
{
    if (Record.FactId.IsNone())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("WorldStateSubsystem: SetWorldStateRecord — FactId is None, record ignored"));
        return;
    }

    FWorldStateRecord* Existing = WorldStateRecords.Find(Record.FactId);

    // Факт "новый", если его не было в карте ИЛИ если он был помечен
    // на удаление и сейчас оживает.
    const bool bIsNewFact = (Existing == nullptr) || Existing->bPendingRemoval;

    // Запоминаем предыдущее значение ДО мутации.
    FString PreviousValue;
    if (!bIsNewFact && Existing)
    {
        PreviousValue = Existing->SerializedValue;
    }

    FWorldStateRecord NewRecord = Record;
    NewRecord.bPendingRemoval = false;

    if (Existing)
    {
        NewRecord.OriginalValue = Existing->OriginalValue;
        NewRecord.bHasOriginalValue = Existing->bHasOriginalValue;
    }

    FWorldStateRecord& Stored = WorldStateRecords.Add(Record.FactId, NewRecord);

    // Применяем к актёру, если он на сцене.
    if (AActor* Actor = FindActorByItemId(Stored.ItemId))
    {
        CaptureOriginalValueIfMissing(Record.FactId, Actor);

        if (const FWorldStateRecord* FinalRecord = WorldStateRecords.Find(Record.FactId))
        {
            ApplyRecordToActor(Actor, *FinalRecord);
        }
    }

    // Снимок для payload делаем ПОСЛЕ применения — OriginalValue уже
    // мог быть захвачен, и подписчик увидит актуальное состояние записи.
    const FWorldStateRecord* Final = WorldStateRecords.Find(Record.FactId);
    const FWorldStateRecord Snapshot = Final ? *Final : Stored;

    UE_LOG(LogTemp, Log,
        TEXT("WorldStateSubsystem: SetRecord FactId='%s' (%s) ItemId=%s Component='%s' Key='%s' Value='%s' Original='%s' (captured=%s)"),
        *Record.FactId.ToString(),
        bIsNewFact ? TEXT("ADDED") : TEXT("CHANGED"),
        *Record.ItemId.ToString(),
        Record.ComponentName.IsNone() ? TEXT("<actor>") : *Record.ComponentName.ToString(),
        *Record.ChangeKey.ToString(),
        *Record.SerializedValue,
        Snapshot.bHasOriginalValue ? *Snapshot.OriginalValue : TEXT("<none>"),
        Snapshot.bHasOriginalValue ? TEXT("true") : TEXT("false"));

    PublishFactEvent(
        Record.FactId,
        Snapshot,
        bIsNewFact ? EOutcomeWorldState::WorldStateFactAdded
        : EOutcomeWorldState::WorldStateFactChanged,
        PreviousValue);
}

void UWorldStateSubsystem::RemoveWorldStateRecord(FName FactId)
{
    FWorldStateRecord* Record = WorldStateRecords.Find(FactId);
    if (!Record)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("WorldStateSubsystem: RemoveRecord FactId='%s' — not found, nothing to do"),
            *FactId.ToString());
        return;
    }

    if (Record->bPendingRemoval)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("WorldStateSubsystem: RemoveRecord FactId='%s' — already pending removal"),
            *FactId.ToString());
        return;
    }

    // Снимок ДО пометки — чтобы подписчик увидел запись "как она была".
    FWorldStateRecord Snapshot = *Record;
    Snapshot.bPendingRemoval = false;

    // Что будет восстановлено на актёре (если оригинал захвачен).
    const bool bHasRestored = Record->bHasOriginalValue;
    const FString RestoredValue = bHasRestored ? Record->OriginalValue : FString();

    Record->bPendingRemoval = true;

    // Событие публикуем сразу — мир считает факт неактивным, даже если
    // восстановление на актёре произойдёт позже (актёра нет на сцене).
    PublishFactEvent(
        FactId,
        Snapshot,
        EOutcomeWorldState::WorldStateFactRemoved,
        /*PreviousValue=*/FString(),
        RestoredValue,
        bHasRestored);

    // Если актёр есть — финализируем немедленно.
    if (AActor* Actor = FindActorByItemId(Record->ItemId))
    {
        if (TryFinalizePendingRemoval(FactId, Actor))
        {
            UE_LOG(LogTemp, Log,
                TEXT("WorldStateSubsystem: Removed record FactId='%s' (restored immediately)"),
                *FactId.ToString());
            return;
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("WorldStateSubsystem: Deferred removal for FactId='%s' (waiting for actor)"),
        *FactId.ToString());
}

void UWorldStateSubsystem::ApplyRecordsToWorld()
{
    TMap<FGuid, AActor*> ActorByItemId;
    BuildActorIndex(ActorByItemId);

    if (ActorByItemId.Num() == 0)
    {
        UE_LOG(LogTemp, Log,
            TEXT("WorldStateSubsystem::ApplyRecordsToWorld: no actors with UFloorAssignmentComponent on current level"));
        return;
    }

    // Собираем факты, разделяя на pending removals и активные.
    TArray<FName> PendingFactIds;
    TArray<FName> ActiveFactIds;

    for (const auto& Pair : WorldStateRecords)
    {
        if (Pair.Value.bPendingRemoval)
            PendingFactIds.Add(Pair.Key);
        else
            ActiveFactIds.Add(Pair.Key);
    }

    int32 Restored = 0;
    int32 Applied = 0;

    // --- Проход 1: pending removals ---
    // Выполняются ДО применения активных фактов, чтобы вернуть свойство
    // в исходное состояние, а затем уже наложить на него текущие факты.
    for (const FName& FactId : PendingFactIds)
    {
        FWorldStateRecord* Record = WorldStateRecords.Find(FactId);
        if (!Record) continue;

        AActor** ActorPtr = ActorByItemId.Find(Record->ItemId);
        if (!ActorPtr || !IsValid(*ActorPtr)) continue;

        if (TryFinalizePendingRemoval(FactId, *ActorPtr))
        {
            ++Restored;
        }
    }

    // --- Проход 2: активные факты ---
    for (const FName& FactId : ActiveFactIds)
    {
        FWorldStateRecord* Record = WorldStateRecords.Find(FactId);
        if (!Record) continue;

        AActor** ActorPtr = ActorByItemId.Find(Record->ItemId);
        if (!ActorPtr || !IsValid(*ActorPtr)) continue;

        AActor* Actor = *ActorPtr;

        CaptureOriginalValueIfMissing(FactId, Actor);

        if (const FWorldStateRecord* FinalRecord = WorldStateRecords.Find(FactId))
        {
            ApplyRecordToActor(Actor, *FinalRecord);
            ++Applied;
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("WorldStateSubsystem::ApplyRecordsToWorld: applied %d records, finalized %d pending removals"),
        Applied, Restored);
}

void UWorldStateSubsystem::ApplyRecordToActor(AActor* Actor, const FWorldStateRecord& Record) const
{
    if (!IsValid(Actor)) return;
    WritePropertyValue(Actor, Record, Record.SerializedValue, /*bIsRestore=*/false);
}

// ─────────────────────────────────────────────────────────────────────────────
// МЕТОДЫ ЧТЕНИЯ (публичные)
// ─────────────────────────────────────────────────────────────────────────────

bool UWorldStateSubsystem::HasWorldStateRecord(FName FactId, bool bIncludePendingRemoval) const
{
    const FWorldStateRecord* Record = WorldStateRecords.Find(FactId);
    if (!Record) return false;
    if (!bIncludePendingRemoval && Record->bPendingRemoval) return false;
    return true;
}

bool UWorldStateSubsystem::GetWorldStateRecord(FName FactId, bool bIncludePendingRemoval, FWorldStateRecord& OutRecord) const
{
    const FWorldStateRecord* Found = WorldStateRecords.Find(FactId);
    if (!Found) return false;
    if (!bIncludePendingRemoval && Found->bPendingRemoval) return false;
    OutRecord = *Found;
    return true;
}

FWorldStateRecord UWorldStateSubsystem::GetWorldStateRecordOrDefault(FName FactId, bool bIncludePendingRemoval) const
{
    const FWorldStateRecord* Found = WorldStateRecords.Find(FactId);
    if (!Found) return FWorldStateRecord();
    if (!bIncludePendingRemoval && Found->bPendingRemoval) return FWorldStateRecord();
    return *Found;
}

TArray<FWorldStateRecord> UWorldStateSubsystem::GetRecordsForItem(const FGuid& ItemId, bool bIncludePendingRemoval) const
{
    TArray<FWorldStateRecord> Result;
    for (const auto& Pair : WorldStateRecords)
    {
        if (Pair.Value.ItemId != ItemId) continue;
        if (!bIncludePendingRemoval && Pair.Value.bPendingRemoval) continue;
        Result.Add(Pair.Value);
    }
    return Result;
}

TArray<FWorldStateRecord> UWorldStateSubsystem::GetRecordsByCategory(EWorldStateChangeCategory Category, bool bIncludePendingRemoval) const
{
    TArray<FWorldStateRecord> Result;
    for (const auto& Pair : WorldStateRecords)
    {
        if (Pair.Value.Category != Category) continue;
        if (!bIncludePendingRemoval && Pair.Value.bPendingRemoval) continue;
        Result.Add(Pair.Value);
    }
    return Result;
}

const FWorldStateRecord* UWorldStateSubsystem::FindWorldStateRecord(FName FactId, bool bIncludePendingRemoval) const
{
    const FWorldStateRecord* Found = WorldStateRecords.Find(FactId);
    if (!Found) return nullptr;
    if (!bIncludePendingRemoval && Found->bPendingRemoval) return nullptr;
    return Found;
}

// ─────────────────────────────────────────────────────────────────────────────
// ISaveableSubsystem
// ─────────────────────────────────────────────────────────────────────────────

void UWorldStateSubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
    OutData.SubsystemName = GetSaveSubsystemName();

    TArray<TSharedPtr<FJsonValue>> RecordsArray;

    for (const auto& Pair : WorldStateRecords)
    {
        const FWorldStateRecord& Rec = Pair.Value;
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("FactId"), Rec.FactId.ToString());
        Obj->SetStringField(TEXT("Description"), Rec.Description);
        Obj->SetNumberField(TEXT("Category"), static_cast<int32>(Rec.Category));
        Obj->SetStringField(TEXT("ItemId"), Rec.ItemId.ToString());
        Obj->SetStringField(TEXT("ComponentName"),
            Rec.ComponentName.IsNone() ? FString() : Rec.ComponentName.ToString());
        Obj->SetStringField(TEXT("ChangeKey"), Rec.ChangeKey.ToString());
        Obj->SetStringField(TEXT("Value"), Rec.SerializedValue);
        Obj->SetStringField(TEXT("OriginalValue"), Rec.OriginalValue);
        Obj->SetBoolField(TEXT("bHasOriginalValue"), Rec.bHasOriginalValue);
        Obj->SetBoolField(TEXT("bPendingRemoval"), Rec.bPendingRemoval);
        Obj->SetStringField(TEXT("ReactionFunctionName"),
            Rec.ReactionFunctionName.IsNone() ? FString() : Rec.ReactionFunctionName.ToString());
        Obj->SetStringField(TEXT("Timestamp"), Rec.Timestamp);
        RecordsArray.Add(MakeShared<FJsonValueObject>(Obj));
    }

    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetArrayField(TEXT("Records"), RecordsArray);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    OutData.SerializedData = Output;

    UE_LOG(LogTemp, Log,
        TEXT("WorldStateSubsystem::CollectSaveData: serialized %d records"),
        RecordsArray.Num());
}

void UWorldStateSubsystem::ApplySaveData(const FSubsystemSaveData& InData)
{
    IsLoadComplete = false;

    if (InData.SerializedData.IsEmpty())
    {
        IsLoadComplete = true;
        return;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InData.SerializedData);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        IsLoadComplete = true;
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* RecordsArray = nullptr;
    if (!Root->TryGetArrayField(TEXT("Records"), RecordsArray))
    {
        IsLoadComplete = true;
        return;
    }

    WorldStateRecords.Empty();

    for (const TSharedPtr<FJsonValue>& Val : *RecordsArray)
    {
        const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
        if (!Val->TryGetObject(ObjPtr)) continue;
        const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

        FString FactIdStr, Description, ItemIdStr, ChangeKeyStr, ComponentNameStr;
        FString Value, OriginalValue, ReactionFunctionNameStr, Timestamp;
        int32 CategoryInt = 0;
        bool bHasOriginalValue = false;
        bool bPendingRemoval = false;

        Obj->TryGetStringField(TEXT("FactId"), FactIdStr);
        Obj->TryGetStringField(TEXT("Description"), Description);
        Obj->TryGetNumberField(TEXT("Category"), CategoryInt);
        Obj->TryGetStringField(TEXT("ItemId"), ItemIdStr);
        Obj->TryGetStringField(TEXT("ChangeKey"), ChangeKeyStr);
        Obj->TryGetStringField(TEXT("ComponentName"), ComponentNameStr);
        Obj->TryGetStringField(TEXT("Value"), Value);
        Obj->TryGetStringField(TEXT("OriginalValue"), OriginalValue);
        Obj->TryGetBoolField(TEXT("bHasOriginalValue"), bHasOriginalValue);
        Obj->TryGetBoolField(TEXT("bPendingRemoval"), bPendingRemoval);
        Obj->TryGetStringField(TEXT("ReactionFunctionName"), ReactionFunctionNameStr);
        Obj->TryGetStringField(TEXT("Timestamp"), Timestamp);

        if (FactIdStr.IsEmpty())
        {
            UE_LOG(LogTemp, Warning,
                TEXT("WorldStateSubsystem::ApplySaveData: record without FactId skipped"));
            continue;
        }

        FGuid ItemId;
        if (!FGuid::Parse(ItemIdStr, ItemId)) continue;

        FWorldStateRecord Record;
        Record.FactId = FName(*FactIdStr);
        Record.Description = Description;
        Record.Category = static_cast<EWorldStateChangeCategory>(CategoryInt);
        Record.ItemId = ItemId;
        Record.ChangeKey = FName(*ChangeKeyStr);
        Record.ComponentName = ComponentNameStr.IsEmpty() ? NAME_None : FName(*ComponentNameStr);
        Record.SerializedValue = Value;
        Record.OriginalValue = OriginalValue;
        Record.bHasOriginalValue = bHasOriginalValue;
        Record.bPendingRemoval = bPendingRemoval;
        Record.ReactionFunctionName = ReactionFunctionNameStr.IsEmpty() ? NAME_None : FName(*ReactionFunctionNameStr);
        Record.Timestamp = Timestamp;

        WorldStateRecords.Add(Record.FactId, Record);
    }

    UE_LOG(LogTemp, Log,
        TEXT("WorldStateSubsystem::ApplySaveData: loaded %d records"),
        WorldStateRecords.Num());

    ApplyRecordsToWorld();

    IsLoadComplete = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// АКТОРЫ: спавн, поиск, индекс
// ─────────────────────────────────────────────────────────────────────────────

void UWorldStateSubsystem::HandleActorSpawned(AActor* SpawnedActor)
{
    if (!IsValid(SpawnedActor))
        return;

    UFloorAssignmentComponent* FAC = SpawnedActor->FindComponentByClass<UFloorAssignmentComponent>();
    if (!FAC || !FAC->ItemId.IsValid())
        return;

    // Собираем факты для этого актёра, разделяя на pending removals и активные.
    TArray<FName> PendingFactIds;
    TArray<FName> ActiveFactIds;

    for (const auto& Pair : WorldStateRecords)
    {
        if (Pair.Value.ItemId != FAC->ItemId) continue;

        if (Pair.Value.bPendingRemoval)
            PendingFactIds.Add(Pair.Key);
        else
            ActiveFactIds.Add(Pair.Key);
    }

    // --- Проход 1: pending removals ---
    for (const FName& FactId : PendingFactIds)
    {
        TryFinalizePendingRemoval(FactId, SpawnedActor);
    }

    // --- Проход 2: активные факты ---
    for (const FName& FactId : ActiveFactIds)
    {
        FWorldStateRecord* Record = WorldStateRecords.Find(FactId);
        if (!Record) continue;

        CaptureOriginalValueIfMissing(FactId, SpawnedActor);

        if (const FWorldStateRecord* FinalRecord = WorldStateRecords.Find(FactId))
        {
            ApplyRecordToActor(SpawnedActor, *FinalRecord);
        }
    }
}

void UWorldStateSubsystem::SubscribeToActorSpawned()
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    if (SubscribedWorld.Get() == World && ActorSpawnedHandle.IsValid())
        return;

    UnsubscribeFromActorSpawned();

    SubscribedWorld = World;
    ActorSpawnedHandle = World->AddOnActorSpawnedHandler(
        FOnActorSpawned::FDelegate::CreateUObject(this, &UWorldStateSubsystem::HandleActorSpawned));
}

void UWorldStateSubsystem::UnsubscribeFromActorSpawned()
{
    if (UWorld* World = SubscribedWorld.Get())
    {
        if (ActorSpawnedHandle.IsValid())
        {
            World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
        }
    }
    ActorSpawnedHandle.Reset();
    SubscribedWorld = nullptr;
}

AActor* UWorldStateSubsystem::FindActorByItemId(const FGuid& ItemId) const
{
    UWorld* World = GetWorld();
    if (!World)
        return nullptr;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor))
            continue;

        if (UFloorAssignmentComponent* FAC = Actor->FindComponentByClass<UFloorAssignmentComponent>())
        {
            if (FAC->ItemId == ItemId)
                return Actor;
        }
    }
    return nullptr;
}

void UWorldStateSubsystem::BuildActorIndex(TMap<FGuid, AActor*>& OutIndex) const
{
    OutIndex.Reset();

    UWorld* World = GetWorld();
    if (!World)
        return;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor))
            continue;

        if (UFloorAssignmentComponent* FAC = Actor->FindComponentByClass<UFloorAssignmentComponent>())
        {
            if (FAC->ItemId.IsValid())
            {
                OutIndex.Add(FAC->ItemId, Actor);
            }
        }
    }
}

UActorComponent* UWorldStateSubsystem::FindComponentByStableName(AActor* Actor, FName ComponentName) const
{
    if (!IsValid(Actor) || ComponentName.IsNone())
        return nullptr;

    TArray<UActorComponent*> Components;
    Actor->GetComponents(Components);

    // 1-й проход: точное совпадение FName (обычно нативные C++-компоненты).
    for (UActorComponent* Comp : Components)
    {
        if (IsValid(Comp) && Comp->GetFName() == ComponentName)
            return Comp;
    }

    // 2-й проход: BP-компоненты получают FName вида "<VariableName>_GEN_VARIABLE".
    static const FString Suffix = TEXT("_GEN_VARIABLE");
    const FString TargetStr = ComponentName.ToString();

    for (UActorComponent* Comp : Components)
    {
        if (!IsValid(Comp)) continue;

        const FString CompNameStr = Comp->GetName();
        if (CompNameStr.Equals(TargetStr, ESearchCase::CaseSensitive))
            return Comp;

        if (CompNameStr.Len() > Suffix.Len()
            && CompNameStr.EndsWith(Suffix, ESearchCase::CaseSensitive))
        {
            const FString WithoutSuffix = CompNameStr.LeftChop(Suffix.Len());
            if (WithoutSuffix.Equals(TargetStr, ESearchCase::CaseSensitive))
                return Comp;
        }
    }

    return nullptr;
}

// ============================================================================
// Level loaded handler
// ============================================================================

void UWorldStateSubsystem::HandleLevelLoaded(const FOutcomeEventBase& Outcome)
{
    ApplyRecordsToWorld();
    SubscribeToActorSpawned();

    UE_LOG(LogTemp, Log,
        TEXT("WorldStateSubsystem::HandleLevelLoaded: reapplied WorldState records for new level"));
}

void UWorldStateSubsystem::CaptureOriginalValueIfMissing(FName FactId, AActor* Actor)
{
    FWorldStateRecord* Record = WorldStateRecords.Find(FactId);
    if (!Record || !IsValid(Actor) || Record->bHasOriginalValue)
        return;

    UObject* Target = nullptr;
    FProperty* Prop = ResolveTargetProperty(Actor, *Record, Target);
    if (!Prop)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("WorldStateSubsystem: CaptureOriginalValueIfMissing — property '%s' NOT found on actor '%s' (FactId='%s')"),
            *Record->ChangeKey.ToString(), *Actor->GetName(), *FactId.ToString());
        return;
    }

    FString Captured;
    if (TryReadPropertyValue(Actor, *Record, Captured))
    {
        Record->OriginalValue = Captured;
        Record->bHasOriginalValue = true;

        UE_LOG(LogTemp, Log,
            TEXT("WorldStateSubsystem: Captured original '%s'='%s' on '%s' (actor '%s', FactId='%s')"),
            *Record->ChangeKey.ToString(), *Captured,
            *Target->GetName(), *Actor->GetName(), *FactId.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("WorldStateSubsystem: CaptureOriginalValueIfMissing — TryRead FAILED for '%s' (FactId='%s')"),
            *Record->ChangeKey.ToString(), *FactId.ToString());
    }
}

FProperty* UWorldStateSubsystem::ResolveTargetProperty(AActor* Actor, const FWorldStateRecord& Record, UObject*& OutTargetObject) const
{
    OutTargetObject = nullptr;
    if (!IsValid(Actor)) return nullptr;

    // --- Вариант 1: указан компонент — ищем свойство строго в нём ---
    if (!Record.ComponentName.IsNone())
    {
        UActorComponent* Comp = FindComponentByStableName(Actor, Record.ComponentName);
        if (!IsValid(Comp)) return nullptr;

        FProperty* Prop = FindFProperty<FProperty>(Comp->GetClass(), Record.ChangeKey);
        if (!Prop || !Prop->HasAllPropertyFlags(CPF_SaveGame)) return nullptr;

        OutTargetObject = Comp;
        return Prop;
    }

    // --- Вариант 2: компонент не указан — ищем свойство на самом актёре ---
    FProperty* Prop = FindFProperty<FProperty>(Actor->GetClass(), Record.ChangeKey);
    if (!Prop || !Prop->HasAllPropertyFlags(CPF_SaveGame)) return nullptr;

    OutTargetObject = Actor;
    return Prop;
}

bool UWorldStateSubsystem::TryReadPropertyValue(AActor* Actor, const FWorldStateRecord& Record, FString& OutValue) const
{
    UObject* Target = nullptr;
    FProperty* Prop = ResolveTargetProperty(Actor, Record, Target);
    if (!Prop || !Target) return false;

    // --- Bool: читаем явно, чтобы false не превратился в "" ---
    if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
    {
        OutValue = BoolProp->GetPropertyValue_InContainer(Target)
            ? TEXT("True") : TEXT("False");
        return true;
    }

    // --- Целочисленные и enum ---
    if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
    {
        OutValue = FString::FromInt(IntProp->GetPropertyValue_InContainer(Target));
        return true;
    }
    if (FInt64Property* Int64Prop = CastField<FInt64Property>(Prop))
    {
        OutValue = FString::Printf(TEXT("%lld"), Int64Prop->GetPropertyValue_InContainer(Target));
        return true;
    }
    if (FInt16Property* Int16Prop = CastField<FInt16Property>(Prop))
    {
        OutValue = FString::FromInt(static_cast<int32>(Int16Prop->GetPropertyValue_InContainer(Target)));
        return true;
    }
    if (FInt8Property* Int8Prop = CastField<FInt8Property>(Prop))
    {
        OutValue = FString::FromInt(static_cast<int32>(Int8Prop->GetPropertyValue_InContainer(Target)));
        return true;
    }
    if (FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
    {
        OutValue = FString::FromInt(static_cast<int32>(ByteProp->GetPropertyValue_InContainer(Target)));
        return true;
    }
    if (FUInt16Property* UInt16Prop = CastField<FUInt16Property>(Prop))
    {
        OutValue = FString::FromInt(static_cast<int32>(UInt16Prop->GetPropertyValue_InContainer(Target)));
        return true;
    }
    if (FUInt32Property* UInt32Prop = CastField<FUInt32Property>(Prop))
    {
        OutValue = FString::Printf(TEXT("%u"), UInt32Prop->GetPropertyValue_InContainer(Target));
        return true;
    }
    if (FUInt64Property* UInt64Prop = CastField<FUInt64Property>(Prop))
    {
        OutValue = FString::Printf(TEXT("%llu"), UInt64Prop->GetPropertyValue_InContainer(Target));
        return true;
    }

    // --- Enum: экспортируем как int32 ---
    if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
    {
        FNumericProperty* Underlying = EnumProp->GetUnderlyingProperty();
        if (Underlying)
        {
            OutValue = FString::FromInt(static_cast<int32>(
                Underlying->GetSignedIntPropertyValue_InContainer(Target)));
            return true;
        }
    }

    // --- Float / Double ---
    if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
    {
        OutValue = FString::SanitizeFloat(
            static_cast<double>(FloatProp->GetPropertyValue_InContainer(Target)));
        return true;
    }
    if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
    {
        OutValue = FString::SanitizeFloat(DoubleProp->GetPropertyValue_InContainer(Target));
        return true;
    }

    // --- Всё остальное: FName, FString, FText, FGuid, структуры, массивы, ... ---
    // Стандартный ExportText. Для этих типов PPF_None работает корректно:
    // пустая строка = пустое значение, ImportText("") восстановит его.
    Prop->ExportText_InContainer(0, OutValue, Target, nullptr, Target, PPF_None);
    return true;
}

bool UWorldStateSubsystem::WritePropertyValue(AActor* Actor, const FWorldStateRecord& Record, const FString& Value, bool bIsRestore) const
{
    UObject* Target = nullptr;
    FProperty* Prop = ResolveTargetProperty(Actor, Record, Target);
    if (!Prop || !Target)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("WorldStateSubsystem: No SaveGame property '%s' found (Component='%s', actor '%s')"),
            *Record.ChangeKey.ToString(),
            Record.ComponentName.IsNone() ? TEXT("<actor>") : *Record.ComponentName.ToString(),
            *Actor->GetName());
        return false;
    }

    if (!Prop->ImportText_InContainer(*Value, Target, Target, PPF_None))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("WorldStateSubsystem: ImportText failed '%s'='%s' on '%s' (actor '%s')"),
            *Record.ChangeKey.ToString(), *Value,
            *Target->GetName(), *Actor->GetName());
        return false;
    }

    UE_LOG(LogTemp, Verbose,
        TEXT("WorldStateSubsystem: %s '%s'='%s' on '%s' (actor '%s')"),
        bIsRestore ? TEXT("Restored") : TEXT("Applied"),
        *Record.ChangeKey.ToString(), *Value,
        *Target->GetName(), *Actor->GetName());

    // Уведомляем цель через рефлексию — без параметров.
    InvokeReactionFunction(Target, Record);

    return true;
}

void UWorldStateSubsystem::HandleSetWorldStateRecord(const FOutcomeEventBase& Outcome)
{
    if (UWorldStateRecordPayload* P = Cast<UWorldStateRecordPayload>(Outcome.Payload))
    {
        SetWorldStateRecord(P->Record);
    }
}

void UWorldStateSubsystem::HandleRemoveWorldStateRecord(const FOutcomeEventBase& Outcome)
{
    if (UWorldStateRecordRemovePayload* P = Cast<UWorldStateRecordRemovePayload>(Outcome.Payload))
    {
        RemoveWorldStateRecord(P->FactId);
    }
}

bool UWorldStateSubsystem::TryFinalizePendingRemoval(FName FactId, AActor* Actor)
{
    FWorldStateRecord* Record = WorldStateRecords.Find(FactId);
    if (!Record || !Record->bPendingRemoval)
        return false;

    if (!IsValid(Actor))
        return false;

    if (!Record->bHasOriginalValue)
    {
        FString Captured;
        if (!TryReadPropertyValue(Actor, *Record, Captured))
        {
            UE_LOG(LogTemp, Verbose,
                TEXT("WorldStateSubsystem: Cannot finalize removal for '%s' on actor '%s' (property not found yet, FactId='%s')"),
                *Record->ChangeKey.ToString(), *Actor->GetName(), *FactId.ToString());
            return false;
        }

        Record->OriginalValue = Captured;
        Record->bHasOriginalValue = true;
    }

    WritePropertyValue(Actor, *Record, Record->OriginalValue, /*bIsRestore=*/true);
    WorldStateRecords.Remove(FactId);

    // Событие Removed уже опубликовано в RemoveWorldStateRecord.
    // Повторно не публикуем — иначе подписчики увидят два Removed на один факт.

    return true;
}

void UWorldStateSubsystem::InvokeReactionFunction(UObject* Target, const FWorldStateRecord& Record) const
{
    if (!IsValid(Target) || Record.ReactionFunctionName.IsNone())
        return;

    UFunction* Func = Target->FindFunction(Record.ReactionFunctionName);
    if (!Func)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("WorldStateSubsystem: ReactionFunction '%s' not found on '%s' (Key='%s')"),
            *Record.ReactionFunctionName.ToString(),
            *Target->GetName(),
            *Record.ChangeKey.ToString());
        return;
    }

    // UFUNCTION без параметров — просто ProcessEvent с nullptr.
    Target->ProcessEvent(Func, nullptr);

    UE_LOG(LogTemp, Verbose,
        TEXT("WorldStateSubsystem: Invoked reaction '%s' on '%s' (Key='%s')"),
        *Record.ReactionFunctionName.ToString(),
        *Target->GetName(),
        *Record.ChangeKey.ToString());
}

void UWorldStateSubsystem::PublishFactEvent(
    FName FactId,
    const FWorldStateRecord& RecordSnapshot,
    EOutcomeWorldState EventType,
    const FString& PreviousValue,
    const FString& RestoredValue,
    bool bHasRestoredValue) const
{
    if (FactId.IsNone())
        return;

    UGameInstance* GI = GetGameInstance();
    if (!GI)
        return;

    UEventBusSubsystem* Bus = GI->GetSubsystem<UEventBusSubsystem>();
    if (!Bus)
        return;

    UWorldStateFactChangedPayload* P = Bus->CreatePayload<UWorldStateFactChangedPayload>();
    if (!P)
        return;

    P->Setup(FactId, RecordSnapshot);
    P->PreviousValue = PreviousValue;
    P->RestoredValue = RestoredValue;
    P->bHasRestoredValue = bHasRestoredValue;

    FOutcomeEventBase Ev;
    Ev.OutcomeType = EOutcomeType::WorldState;
    Ev.OutcomeWorldState = EventType;
    Ev.Payload = P;

    Bus->PublishOutcome(Ev);
}
