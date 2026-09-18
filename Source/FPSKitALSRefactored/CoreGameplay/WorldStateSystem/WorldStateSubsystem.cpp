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

// ─────────────────────────────────────────────────────────────────────────────
// WORLD STATE RECORDS — ПРИВАТНЫЕ МЕТОДЫ ИЗМЕНЕНИЯ
// ─────────────────────────────────────────────────────────────────────────────

void UWorldStateSubsystem::SetWorldStateRecord(const FWorldStateRecord& Record)
{
    const FWorldStateKey Key(Record.ItemId, Record.ComponentName, Record.ChangeKey);
    FWorldStateRecord* Existing = WorldStateRecords.Find(Key);

    FWorldStateRecord NewRecord = Record;

    // Новый Set отменяет отложенное удаление.
    NewRecord.bPendingRemoval = false;

    if (Existing)
    {
        // Сохраняем уже захваченный оригинал (если был).
        NewRecord.OriginalValue = Existing->OriginalValue;
        NewRecord.bHasOriginalValue = Existing->bHasOriginalValue;
    }

    WorldStateRecords.Add(Key, NewRecord);

    // Немедленно применяем к живому актёру, если он есть.
    if (AActor* Actor = FindActorByItemId(Record.ItemId))
    {
        CaptureOriginalValueIfMissing(Key, Actor);

        if (const FWorldStateRecord* FinalRecord = WorldStateRecords.Find(Key))
        {
            ApplyRecordToActor(Actor, *FinalRecord);
        }
    }

    const FWorldStateRecord* Final = WorldStateRecords.Find(Key);
    UE_LOG(LogTemp, Log,
        TEXT("WorldStateSubsystem: SetRecord ItemId=%s Component='%s' Key='%s' Value='%s' Original='%s' (captured=%s)"),
        *Record.ItemId.ToString(),
        Record.ComponentName.IsNone() ? TEXT("<actor>") : *Record.ComponentName.ToString(),
        *Record.ChangeKey.ToString(),
        *Record.SerializedValue,
        (Final && Final->bHasOriginalValue) ? *Final->OriginalValue : TEXT("<none>"),
        (Final && Final->bHasOriginalValue) ? TEXT("true") : TEXT("false"));
}

void UWorldStateSubsystem::RemoveWorldStateRecord(const FGuid& ItemId, FName ComponentName, FName ChangeKey)
{
    const FWorldStateKey Key(ItemId, ComponentName, ChangeKey);
    FWorldStateRecord* Record = WorldStateRecords.Find(Key);
    if (!Record) return;

    // Помечаем запись как ожидающую удаления. Даже если сейчас сможем
    // финализировать — снаружи поведение одинаково: запись исчезнет только
    // после восстановления оригинала.
    Record->bPendingRemoval = true;

    // Если актёр доступен — пытаемся финализировать немедленно.
    if (AActor* Actor = FindActorByItemId(ItemId))
    {
        if (TryFinalizePendingRemoval(Key, Actor))
        {
            UE_LOG(LogTemp, Log,
                TEXT("WorldStateSubsystem: Removed record ItemId=%s Component='%s' Key='%s' (restored immediately)"),
                *ItemId.ToString(),
                ComponentName.IsNone() ? TEXT("<actor>") : *ComponentName.ToString(),
                *ChangeKey.ToString());
            return;
        }
    }

    // Актёра нет — финализируем, когда он появится.
    UE_LOG(LogTemp, Log,
        TEXT("WorldStateSubsystem: Deferred removal for ItemId=%s Component='%s' Key='%s' (waiting for actor)"),
        *ItemId.ToString(),
        ComponentName.IsNone() ? TEXT("<actor>") : *ComponentName.ToString(),
        *ChangeKey.ToString());
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

    // Копируем ключи: в процессе итерации карта может мутировать
    // (TryFinalizePendingRemoval удаляет записи).
    TArray<FWorldStateKey> Keys;
    WorldStateRecords.GetKeys(Keys);

    int32 Applied = 0;
    int32 Restored = 0;

    for (const FWorldStateKey& Key : Keys)
    {
        AActor** ActorPtr = ActorByItemId.Find(Key.ItemId);
        if (!ActorPtr || !IsValid(*ActorPtr))
            continue;

        AActor* Actor = *ActorPtr;

        FWorldStateRecord* Record = WorldStateRecords.Find(Key);
        if (!Record) continue;

        // Отложенное удаление — пытаемся финализировать.
        if (Record->bPendingRemoval)
        {
            if (TryFinalizePendingRemoval(Key, Actor))
            {
                ++Restored;
            }
            continue;
        }

        // Обычная логика применения.
        CaptureOriginalValueIfMissing(Key, Actor);

        if (const FWorldStateRecord* FinalRecord = WorldStateRecords.Find(Key))
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

bool UWorldStateSubsystem::HasWorldStateRecord(const FGuid& ItemId, FName ComponentName, FName ChangeKey) const
{
    const FWorldStateKey Key(ItemId, ComponentName, ChangeKey);
    const FWorldStateRecord* Record = WorldStateRecords.Find(Key);
    return Record && !Record->bPendingRemoval;
}

bool UWorldStateSubsystem::GetWorldStateRecord(const FGuid& ItemId, FName ComponentName, FName ChangeKey, FWorldStateRecord& OutRecord) const
{
    const FWorldStateKey Key(ItemId, ComponentName, ChangeKey);
    const FWorldStateRecord* Found = WorldStateRecords.Find(Key);
    if (!Found || Found->bPendingRemoval)
        return false;
    OutRecord = *Found;
    return true;
}

TArray<FWorldStateRecord> UWorldStateSubsystem::GetRecordsForItem(const FGuid& ItemId) const
{
    TArray<FWorldStateRecord> Result;
    for (const auto& Pair : WorldStateRecords)
    {
        if (Pair.Key.ItemId == ItemId && !Pair.Value.bPendingRemoval)
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
        if (Pair.Value.Category == Category && !Pair.Value.bPendingRemoval)
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

        FString ItemIdStr, ChangeKeyStr, ComponentNameStr, Value, OriginalValue, ReactionFunctionNameStr, Timestamp;
        int32 CategoryInt = 0;
        bool bHasOriginalValue = false;
        bool bPendingRemoval = false;

        Obj->TryGetStringField(TEXT("ItemId"), ItemIdStr);
        Obj->TryGetNumberField(TEXT("Category"), CategoryInt);
        Obj->TryGetStringField(TEXT("ChangeKey"), ChangeKeyStr);
        Obj->TryGetStringField(TEXT("ComponentName"), ComponentNameStr);
        Obj->TryGetStringField(TEXT("Value"), Value);
        Obj->TryGetStringField(TEXT("OriginalValue"), OriginalValue);
        Obj->TryGetBoolField(TEXT("bHasOriginalValue"), bHasOriginalValue);
        Obj->TryGetBoolField(TEXT("bPendingRemoval"), bPendingRemoval);
        Obj->TryGetStringField(TEXT("ReactionFunctionName"), ReactionFunctionNameStr);
        Obj->TryGetStringField(TEXT("Timestamp"), Timestamp);

        FGuid ItemId;
        if (!FGuid::Parse(ItemIdStr, ItemId)) continue;

        FWorldStateRecord Record;
        Record.ItemId = ItemId;
        Record.Category = static_cast<EWorldStateChangeCategory>(CategoryInt);
        Record.ChangeKey = FName(*ChangeKeyStr);
        Record.ComponentName = ComponentNameStr.IsEmpty() ? NAME_None : FName(*ComponentNameStr);
        Record.SerializedValue = Value;
        Record.OriginalValue = OriginalValue;
        Record.bHasOriginalValue = bHasOriginalValue;
        Record.bPendingRemoval = bPendingRemoval;
        Record.ReactionFunctionName = ReactionFunctionNameStr.IsEmpty() ? NAME_None : FName(*ReactionFunctionNameStr);
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

    TArray<FWorldStateKey> Keys;
    WorldStateRecords.GetKeys(Keys);

    for (const FWorldStateKey& Key : Keys)
    {
        if (Key.ItemId != FAC->ItemId)
            continue;

        FWorldStateRecord* Record = WorldStateRecords.Find(Key);
        if (!Record) continue;

        // Отложенное удаление — финализируем, НЕ применяя SerializedValue.
        if (Record->bPendingRemoval)
        {
            TryFinalizePendingRemoval(Key, SpawnedActor);
            continue;
        }

        // Обычный путь: захватываем оригинал и применяем значение.
        CaptureOriginalValueIfMissing(Key, SpawnedActor);

        if (const FWorldStateRecord* FinalRecord = WorldStateRecords.Find(Key))
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

void UWorldStateSubsystem::CaptureOriginalValueIfMissing(const FWorldStateKey& Key, AActor* Actor)
{
    FWorldStateRecord* Record = WorldStateRecords.Find(Key);
    if (!Record || !IsValid(Actor) || Record->bHasOriginalValue)
        return;

    FString Captured;
    if (TryReadPropertyValue(Actor, *Record, Captured))
    {
        Record->OriginalValue = Captured;
        Record->bHasOriginalValue = true;

        UE_LOG(LogTemp, Verbose,
            TEXT("WorldStateSubsystem: Captured original '%s'='%s' on actor '%s' (Component='%s')"),
            *Record->ChangeKey.ToString(), *Captured,
            *Actor->GetName(),
            Record->ComponentName.IsNone() ? TEXT("<actor>") : *Record->ComponentName.ToString());
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
        RemoveWorldStateRecord(P->ItemId, P->ComponentName, P->ChangeKey);
    }
}

bool UWorldStateSubsystem::TryFinalizePendingRemoval(const FWorldStateKey& Key, AActor* Actor)
{
    FWorldStateRecord* Record = WorldStateRecords.Find(Key);
    if (!Record || !Record->bPendingRemoval)
        return false;

    if (!IsValid(Actor))
        return false;

    // Если оригинал не захвачен — захватываем текущее значение как оригинал.
    // Это корректно при первом появлении актёра (значение уровня/класса).
    // Если свойство не найдено — финализировать нельзя, ждём дальше.
    if (!Record->bHasOriginalValue)
    {
        FString Captured;
        if (!TryReadPropertyValue(Actor, *Record, Captured))
        {
            UE_LOG(LogTemp, Verbose,
                TEXT("WorldStateSubsystem: Cannot finalize removal for '%s' on actor '%s' (property not found yet)"),
                *Record->ChangeKey.ToString(), *Actor->GetName());
            return false;
        }

        Record->OriginalValue = Captured;
        Record->bHasOriginalValue = true;

        UE_LOG(LogTemp, Verbose,
            TEXT("WorldStateSubsystem: Captured original '%s'='%s' on actor '%s' (before removing record)"),
            *Record->ChangeKey.ToString(), *Captured, *Actor->GetName());
    }

    // Восстанавливаем оригинал и удаляем запись.
    WritePropertyValue(Actor, *Record, Record->OriginalValue, /*bIsRestore=*/true);
    WorldStateRecords.Remove(Key);
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
