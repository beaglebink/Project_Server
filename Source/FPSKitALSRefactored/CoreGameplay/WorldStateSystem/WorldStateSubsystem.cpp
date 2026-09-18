#include "WorldStateSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "ChangingLocationAvailabilityPayload.h"
#include "../InteriorInstanceSystem/FloorAssignmentComponent.h"
#include "../SaveGame/GameSaveSubsystem.h"
#include "EngineUtils.h"
#include "JsonObjectConverter.h"
#include "WorldStateRecordPayload.h"
#include "WorldStateRecordRemovePayload.h"
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
        RemoveWorldStateRecord(P->ItemId, P->ComponentName, P->ChangeKey);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// WORLD STATE RECORDS — ПРИВАТНЫЕ МЕТОДЫ ИЗМЕНЕНИЯ
// ─────────────────────────────────────────────────────────────────────────────

void UWorldStateSubsystem::SetWorldStateRecord(const FWorldStateRecord& Record)
{
    const FWorldStateKey Key(Record.ItemId, Record.ComponentName, Record.ChangeKey);
    WorldStateRecords.Add(Key, Record);

    // Немедленно применяем к живому актёру, если он уже есть на текущей сцене.
    if (AActor* Actor = FindActorByItemId(Record.ItemId))
    {
        ApplyRecordToActor(Actor, Record);
    }

    UE_LOG(LogTemp, Log,
        TEXT("WorldStateSubsystem: SetRecord ItemId=%s Component='%s' Key='%s' Value='%s'"),
        *Record.ItemId.ToString(),
        Record.ComponentName.IsNone() ? TEXT("<actor>") : *Record.ComponentName.ToString(),
        *Record.ChangeKey.ToString(),
        *Record.SerializedValue);
}

void UWorldStateSubsystem::RemoveWorldStateRecord(const FGuid& ItemId, FName ComponentName, FName ChangeKey)
{
    const FWorldStateKey Key(ItemId, ComponentName, ChangeKey);
    if (WorldStateRecords.Remove(Key) > 0)
    {
        UE_LOG(LogTemp, Log,
            TEXT("WorldStateSubsystem: Removed record ItemId=%s Component='%s' Key='%s'"),
            *ItemId.ToString(),
            ComponentName.IsNone() ? TEXT("<actor>") : *ComponentName.ToString(),
            *ChangeKey.ToString());
    }
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

    int32 Applied = 0;
    for (const auto& Pair : WorldStateRecords)
    {
        const FWorldStateKey& Key = Pair.Key;
        const FWorldStateRecord& Record = Pair.Value;

        AActor** ActorPtr = ActorByItemId.Find(Key.ItemId);
        if (!ActorPtr || !IsValid(*ActorPtr))
            continue;

        ApplyRecordToActor(*ActorPtr, Record);
        ++Applied;
    }

    UE_LOG(LogTemp, Log,
        TEXT("WorldStateSubsystem::ApplyRecordsToWorld: applied %d records"), Applied);
}

void UWorldStateSubsystem::ApplyRecordToActor(AActor* Actor, const FWorldStateRecord& Record) const
{
    if (!IsValid(Actor)) return;

    // --- Вариант 1: указан компонент — ищем свойство строго в нём ---
    if (!Record.ComponentName.IsNone())
    {
        UActorComponent* Comp = FindComponentByStableName(Actor, Record.ComponentName);
        if (!IsValid(Comp))
        {
            UE_LOG(LogTemp, Verbose,
                TEXT("WorldStateSubsystem: Component '%s' not found on actor '%s' (Key='%s')"),
                *Record.ComponentName.ToString(), *Actor->GetName(), *Record.ChangeKey.ToString());
            return;
        }

        FProperty* CompProp = FindFProperty<FProperty>(Comp->GetClass(), Record.ChangeKey);
        if (CompProp && CompProp->HasAllPropertyFlags(CPF_SaveGame))
        {
            if (!CompProp->ImportText_InContainer(*Record.SerializedValue, Comp, Comp, PPF_None))
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("WorldStateSubsystem: ImportText failed '%s'='%s' on component '%s' (actor '%s')"),
                    *Record.ChangeKey.ToString(), *Record.SerializedValue,
                    *Comp->GetName(), *Actor->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Verbose,
                    TEXT("WorldStateSubsystem: Applied '%s'='%s' to component '%s' on actor '%s'"),
                    *Record.ChangeKey.ToString(), *Record.SerializedValue,
                    *Comp->GetName(), *Actor->GetName());
            }
        }
        else
        {
            UE_LOG(LogTemp, Verbose,
                TEXT("WorldStateSubsystem: No SaveGame property '%s' on component '%s' (actor '%s')"),
                *Record.ChangeKey.ToString(), *Comp->GetName(), *Actor->GetName());
        }
        return;
    }

    // --- Вариант 2: компонент не указан — ищем свойство на самом актёре ---
    FProperty* Prop = FindFProperty<FProperty>(Actor->GetClass(), Record.ChangeKey);
    if (Prop && Prop->HasAllPropertyFlags(CPF_SaveGame))
    {
        if (!Prop->ImportText_InContainer(*Record.SerializedValue, Actor, Actor, PPF_None))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("WorldStateSubsystem: ImportText failed '%s'='%s' on actor '%s'"),
                *Record.ChangeKey.ToString(), *Record.SerializedValue, *Actor->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Verbose,
                TEXT("WorldStateSubsystem: Applied '%s'='%s' to actor '%s'"),
                *Record.ChangeKey.ToString(), *Record.SerializedValue, *Actor->GetName());
        }
        return;
    }

    UE_LOG(LogTemp, Verbose,
        TEXT("WorldStateSubsystem: No SaveGame property '%s' found on actor '%s'"),
        *Record.ChangeKey.ToString(), *Actor->GetName());
}

// ─────────────────────────────────────────────────────────────────────────────
// МЕТОДЫ ЧТЕНИЯ (публичные)
// ─────────────────────────────────────────────────────────────────────────────

bool UWorldStateSubsystem::HasWorldStateRecord(const FGuid& ItemId, FName ComponentName, FName ChangeKey) const
{
    const FWorldStateKey Key(ItemId, ComponentName, ChangeKey);
    return WorldStateRecords.Contains(Key);
}

bool UWorldStateSubsystem::GetWorldStateRecord(const FGuid& ItemId, FName ComponentName, FName ChangeKey, FWorldStateRecord& OutRecord) const
{
    const FWorldStateKey Key(ItemId, ComponentName, ChangeKey);
    const FWorldStateRecord* Found = WorldStateRecords.Find(Key);
    if (!Found)
        return false;
    OutRecord = *Found;
    return true;
}

TArray<FWorldStateRecord> UWorldStateSubsystem::GetRecordsForItem(const FGuid& ItemId) const
{
    TArray<FWorldStateRecord> Result;
    for (const auto& Pair : WorldStateRecords)
    {
        if (Pair.Key.ItemId == ItemId)
        {
            Result.Add(Pair.Value);
        }
    }
    return Result;
}

TArray<FWorldStateRecord> UWorldStateSubsystem::GetRecordsByCategory(EWorldStateChangeCategory Category) const
{
    TArray<FWorldStateRecord> Result;
    for (const auto& Pair : WorldStateRecords)
    {
        if (Pair.Value.Category == Category)
        {
            Result.Add(Pair.Value);
        }
    }
    return Result;
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
        Obj->SetStringField(TEXT("ItemId"), Rec.ItemId.ToString());
        Obj->SetNumberField(TEXT("Category"), static_cast<int32>(Rec.Category));
        Obj->SetStringField(TEXT("ComponentName"),
            Rec.ComponentName.IsNone() ? FString() : Rec.ComponentName.ToString());
        Obj->SetStringField(TEXT("ChangeKey"), Rec.ChangeKey.ToString());
        Obj->SetStringField(TEXT("Value"), Rec.SerializedValue);
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

        FString ItemIdStr, ChangeKeyStr, ComponentNameStr, Value, Timestamp;
        int32 CategoryInt = 0;

        Obj->TryGetStringField(TEXT("ItemId"), ItemIdStr);
        Obj->TryGetNumberField(TEXT("Category"), CategoryInt);
        Obj->TryGetStringField(TEXT("ChangeKey"), ChangeKeyStr);
        Obj->TryGetStringField(TEXT("ComponentName"), ComponentNameStr);
        Obj->TryGetStringField(TEXT("Value"), Value);
        Obj->TryGetStringField(TEXT("Timestamp"), Timestamp);

        FGuid ItemId;
        if (!FGuid::Parse(ItemIdStr, ItemId)) continue;

        FWorldStateRecord Record;
        Record.ItemId = ItemId;
        Record.Category = static_cast<EWorldStateChangeCategory>(CategoryInt);
        Record.ChangeKey = FName(*ChangeKeyStr);
        Record.ComponentName = ComponentNameStr.IsEmpty() ? NAME_None : FName(*ComponentNameStr);
        Record.SerializedValue = Value;
        Record.Timestamp = Timestamp;

        const FWorldStateKey Key(Record.ItemId, Record.ComponentName, Record.ChangeKey);
        WorldStateRecords.Add(Key, Record);
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

    // Применяем все записи, относящиеся к этому ItemId (по любому ComponentName).
    for (const auto& Pair : WorldStateRecords)
    {
        if (Pair.Key.ItemId == FAC->ItemId)
        {
            ApplyRecordToActor(SpawnedActor, Pair.Value);
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