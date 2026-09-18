#include "WorldStateSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "ChangingLocationAvailabilityPayload.h"
#include "../InteriorInstanceSystem/FloorAssignmentComponent.h"
#include "../SaveGame/GameSaveSubsystem.h"
#include "EngineUtils.h"
#include "JsonObjectConverter.h"
#include "WorldStateRecordPayload.h"
#include "WorldStateRecordRemovePayload.h"

void UWorldStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Форсируем инициализацию системы сохранения ДО нас.
    // Без этого GetSubsystem<UGameSaveSubsystem>() может вернуть nullptr,
    // и регистрация Saveable-подсистемы молча не сработает.
    Collection.InitializeDependency<UGameSaveSubsystem>();
	
    // Регистрируемся в GameSaveSubsystem
    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->RegisterSaveableSubsystem(this);
    }

    SubscribeAllWorldStateEvents();
}

void UWorldStateSubsystem::SubscribeAllWorldStateEvents()
{
    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (EventBus)
    {
        WorldStateAddRecordCondition = CreateSimpleWorldStateCondition(EOutcomeWorldState::WorldStateAddRecord);

        WorldStateRecordHandle = EventBus->RegisterHandler(
            WorldStateAddRecordCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UWorldStateSubsystem::HandleSetWorldStateRecord)
        );

        WorldStateRemoveRecordCondition = CreateSimpleWorldStateCondition(EOutcomeWorldState::WorldStateRemoveRecord);

        WorldStateRecordRemoveHandle = EventBus->RegisterHandler(
            WorldStateRemoveRecordCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UWorldStateSubsystem::HandleRemoveWorldStateRecord)
		);
    }
}

void UWorldStateSubsystem::Deinitialize()
{
    UnsubscribeAll();

    // Снимаем регистрацию
    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->UnregisterSaveableSubsystem(this);
    }

    // Дополнительная очистка
    WorldStateRecords.Empty();
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

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (EventBus)
    {
        auto Unreg = [&](FOutcomeHandlerHandle& Handle) {
            if (Handle.IsValid()) { EventBus->UnregisterHandler(Handle); Handle.Invalidate(); }
            };
        Unreg(WorldStateRecordHandle);
        Unreg(WorldStateRecordRemoveHandle);
    }

    //UnsubscribeChangingLocationAvailability();
    /*
    if (WorldStateRecordHandle.IsValid())
    {
        if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
        {
            EventBus->UnregisterHandler(WorldStateRecordHandle);
        }
        WorldStateRecordHandle.Invalidate();
    }

    if (WorldStateRecordRemoveHandle.IsValid())
    {
        if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
        {
            EventBus->UnregisterHandler(WorldStateRecordRemoveHandle);
        }
        WorldStateRecordRemoveHandle.Invalidate();
    }
    */
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
        RemoveWorldStateRecord(P->ItemId, P->ChangeKey);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// WORLD STATE RECORDS — ПРИВАТНЫЕ МЕТОДЫ ИЗМЕНЕНИЯ
// ─────────────────────────────────────────────────────────────────────────────

void UWorldStateSubsystem::SetWorldStateRecord(const FWorldStateRecord& Record)
{
    TMap<FName, FWorldStateRecord>& Inner = WorldStateRecords.FindOrAdd(Record.ItemId);
    Inner.Add(Record.ChangeKey, Record);

    UE_LOG(LogTemp, Log,
        TEXT("WorldStateSubsystem: SetRecord ItemId=%s Key='%s' Value='%s' Mission='%s'"),
        *Record.ItemId.ToString(),
        *Record.ChangeKey.ToString(),
        *Record.SerializedValue,
        *Record.SourceMissionId.ToString());
}

void UWorldStateSubsystem::RemoveWorldStateRecord(const FGuid& ItemId, FName ChangeKey)
{
    TMap<FName, FWorldStateRecord>* Inner = WorldStateRecords.Find(ItemId);
    if (!Inner) return;
    Inner->Remove(ChangeKey);
    if (Inner->IsEmpty())
    {
        WorldStateRecords.Remove(ItemId);
    }
    UE_LOG(LogTemp, Log, TEXT("WorldStateSubsystem: Removed record ItemId=%s Key='%s'"), *ItemId.ToString(), *ChangeKey.ToString());
}

void UWorldStateSubsystem::ApplyRecordsToWorld()
{
    UWorld* World = GetWorld();
    if (!World) return;

    TMap<FGuid, AActor*> ActorByItemId;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor)) continue;
        if (UFloorAssignmentComponent* FAC = Actor->FindComponentByClass<UFloorAssignmentComponent>())
        {
            if (FAC->ItemId.IsValid())
            {
                ActorByItemId.Add(FAC->ItemId, Actor);
            }
        }
    }

    int32 Applied = 0;
    for (const auto& OuterPair : WorldStateRecords)
    {
        AActor** ActorPtr = ActorByItemId.Find(OuterPair.Key);
        if (!ActorPtr || !IsValid(*ActorPtr)) continue;
        AActor* Actor = *ActorPtr;

        for (const auto& InnerPair : OuterPair.Value)
        {
            ApplyRecordToActor(Actor, InnerPair.Value);
            ++Applied;
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("WorldStateSubsystem::ApplyRecordsToWorld: applied %d records"), Applied);
}

void UWorldStateSubsystem::ApplyRecordToActor(AActor* Actor, const FWorldStateRecord& Record) const
{
    if (!IsValid(Actor)) return;

    FProperty* Prop = FindFProperty<FProperty>(Actor->GetClass(), Record.ChangeKey);
    if (Prop && Prop->HasAllPropertyFlags(CPF_SaveGame))
    {
        Prop->ImportText_InContainer(*Record.SerializedValue, Actor, Actor, PPF_None);
        UE_LOG(LogTemp, Verbose,
            TEXT("WorldStateSubsystem: Applied '%s'='%s' to actor '%s'"),
            *Record.ChangeKey.ToString(), *Record.SerializedValue, *Actor->GetName());
        return;
    }

    TArray<UActorComponent*> Components;
    Actor->GetComponents(Components);
    for (UActorComponent* Comp : Components)
    {
        if (!IsValid(Comp)) continue;
        FProperty* CompProp = FindFProperty<FProperty>(Comp->GetClass(), Record.ChangeKey);
        if (CompProp && CompProp->HasAllPropertyFlags(CPF_SaveGame))
        {
            CompProp->ImportText_InContainer(*Record.SerializedValue, Comp, Comp, PPF_None);
            UE_LOG(LogTemp, Verbose,
                TEXT("WorldStateSubsystem: Applied '%s'='%s' to component '%s' on actor '%s'"),
                *Record.ChangeKey.ToString(), *Record.SerializedValue,
                *Comp->GetName(), *Actor->GetName());
            return;
        }
    }

    UE_LOG(LogTemp, Verbose,
        TEXT("WorldStateSubsystem: No SaveGame property '%s' found on actor '%s'"),
        *Record.ChangeKey.ToString(), *Actor->GetName());
}

// ─────────────────────────────────────────────────────────────────────────────
// МЕТОДЫ ЧТЕНИЯ (публичные)
// ─────────────────────────────────────────────────────────────────────────────

bool UWorldStateSubsystem::HasWorldStateRecord(const FGuid& ItemId, FName ChangeKey) const
{
    const TMap<FName, FWorldStateRecord>* Inner = WorldStateRecords.Find(ItemId);
    if (!Inner) return false;
    return Inner->Contains(ChangeKey);
}

bool UWorldStateSubsystem::GetWorldStateRecord(const FGuid& ItemId, FName ChangeKey, FWorldStateRecord& OutRecord) const
{
    const TMap<FName, FWorldStateRecord>* Inner = WorldStateRecords.Find(ItemId);
    if (!Inner) return false;
    const FWorldStateRecord* Found = Inner->Find(ChangeKey);
    if (!Found) return false;
    OutRecord = *Found;
    return true;
}

TArray<FWorldStateRecord> UWorldStateSubsystem::GetRecordsForItem(const FGuid& ItemId) const
{
    TArray<FWorldStateRecord> Result;
    const TMap<FName, FWorldStateRecord>* Inner = WorldStateRecords.Find(ItemId);
    if (!Inner) return Result;
    for (const auto& Pair : *Inner)
    {
        Result.Add(Pair.Value);
    }
    return Result;
}

TArray<FWorldStateRecord> UWorldStateSubsystem::GetRecordsByCategory(EWorldStateChangeCategory Category) const
{
    TArray<FWorldStateRecord> Result;
    for (const auto& OuterPair : WorldStateRecords)
    {
        for (const auto& InnerPair : OuterPair.Value)
        {
            if (InnerPair.Value.Category == Category)
            {
                Result.Add(InnerPair.Value);
            }
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

    for (const auto& OuterPair : WorldStateRecords)
    {
        for (const auto& InnerPair : OuterPair.Value)
        {
            const FWorldStateRecord& Rec = InnerPair.Value;
            TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
            Obj->SetStringField(TEXT("ItemId"), Rec.ItemId.ToString());
            Obj->SetNumberField(TEXT("Category"), static_cast<int32>(Rec.Category));
            Obj->SetStringField(TEXT("ChangeKey"), Rec.ChangeKey.ToString());
            Obj->SetStringField(TEXT("Value"), Rec.SerializedValue);
            Obj->SetStringField(TEXT("MissionId"), Rec.SourceMissionId.ToString());
            Obj->SetStringField(TEXT("Timestamp"), Rec.Timestamp);
            RecordsArray.Add(MakeShared<FJsonValueObject>(Obj));
        }
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

    if (InData.SerializedData.IsEmpty()) return;

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InData.SerializedData);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return;

    const TArray<TSharedPtr<FJsonValue>>* RecordsArray = nullptr;
    if (!Root->TryGetArrayField(TEXT("Records"), RecordsArray)) return;

    WorldStateRecords.Empty();

    for (const TSharedPtr<FJsonValue>& Val : *RecordsArray)
    {
        const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
        if (!Val->TryGetObject(ObjPtr)) continue;
        const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

        FString ItemIdStr, ChangeKeyStr, Value, MissionIdStr, Timestamp;
        int32 CategoryInt = 0;

        Obj->TryGetStringField(TEXT("ItemId"), ItemIdStr);
        Obj->TryGetNumberField(TEXT("Category"), CategoryInt);
        Obj->TryGetStringField(TEXT("ChangeKey"), ChangeKeyStr);
        Obj->TryGetStringField(TEXT("Value"), Value);
        Obj->TryGetStringField(TEXT("MissionId"), MissionIdStr);
        Obj->TryGetStringField(TEXT("Timestamp"), Timestamp);

        FGuid ItemId;
        if (!FGuid::Parse(ItemIdStr, ItemId)) continue;

        FWorldStateRecord Record;
        Record.ItemId = ItemId;
        Record.Category = static_cast<EWorldStateChangeCategory>(CategoryInt);
        Record.ChangeKey = FName(*ChangeKeyStr);
        Record.SerializedValue = Value;
        Record.SourceMissionId = FName(*MissionIdStr);
        Record.Timestamp = Timestamp;

        WorldStateRecords.FindOrAdd(ItemId).Add(Record.ChangeKey, Record);
    }

    UE_LOG(LogTemp, Log,
        TEXT("WorldStateSubsystem::ApplySaveData: loaded %d items with records"),
        WorldStateRecords.Num());

    ApplyRecordsToWorld();

    IsLoadComplete = true;
}