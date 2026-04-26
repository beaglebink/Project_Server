#include "WorldStateSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "ChangingLocationAvailabilityPayload.h"
#include "../InteriorInstanceSystem/FloorAssignmentComponent.h"
#include "../SaveGame/GameSaveSubsystem.h"
#include "EngineUtils.h"
#include "JsonObjectConverter.h"
#include "WorldStateRecordPayload.h"

void UWorldStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Регистрируемся в GameSaveSubsystem
	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->RegisterSaveableSubsystem(this);
	}

	if (ChangingLocationAvailabilityCondition)
	{
		SubscribeChangingLocationAvailability();
	}

	// subscribe to incoming world-state record commands (if condition asset provided)
	if (WorldStateRecordCondition)
	{
		if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
		{
			WorldStateRecordHandle = EventBus->RegisterHandler(
				WorldStateRecordCondition,
				FOutcomeHandlerDelegate::CreateUObject(this, &UWorldStateSubsystem::HandleSetWorldStateRecord));
		}
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

	WorldStateRecords.Empty();
	Super::Deinitialize();
}

void UWorldStateSubsystem::SetChangingLocationAvailabilityCondition(UOutcomeConditionAsset* NewCondition)
{
	// If the condition did not change, do nothing.
	// (Если условие не изменилось — ничего не делаем.)
	if (ChangingLocationAvailabilityCondition == NewCondition) return;

	// Save the new condition.
	// (Сохраняем новое условие.)
	ChangingLocationAvailabilityCondition = NewCondition;

	// If already subscribed, update registration.
	// (Если уже подписаны — обновляем регистрацию.)
	if (ChangingLocationAvailabilityHandle.IsValid())
	{
		UnsubscribeChangingLocationAvailability();
	}

	// Try to subscribe immediately if EventBus is available.
	// (Попробуем подписаться сразу, если EventBus доступен.)
	SubscribeChangingLocationAvailability();
}

void UWorldStateSubsystem::SubscribeChangingLocationAvailability()
{
	// Already subscribed — nothing to do.
	// (Уже подписаны — ничего делать не нужно.)
	if (ChangingLocationAvailabilityHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("WorldStateSubsystem: Already subscribed to ChangingLocationAvailability"));
		return;
	}

	// Condition asset is required to subscribe.
	// (Для подписки требуется ассет условия.)
	if (!ChangingLocationAvailabilityCondition)
	{
		UE_LOG(LogTemp, Warning, TEXT("WorldStateSubsystem: ChangingLocationAvailabilityCondition is null, cannot subscribe (call SetChangingLocationAvailabilityCondition after assigning asset)"));
		return;
	}

	// Register handler with EventBus subsystem if available.
	// (Регистрируем обработчик в EventBus, если он доступен.)
	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		ChangingLocationAvailabilityHandle = EventBus->RegisterHandler(
			ChangingLocationAvailabilityCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UWorldStateSubsystem::HandleChangingLocationAvailability)
		);

		UE_LOG(LogTemp, Log, TEXT("WorldStateSubsystem: Subscribed to ChangingLocationAvailability (handle=%u)"), ChangingLocationAvailabilityHandle.GetId());
	}
}

void UWorldStateSubsystem::UnsubscribeChangingLocationAvailability()
{
	// If handle is invalid — nothing to unsubscribe.
	// (Если дескриптор невалидный — нечего отписывать.)
	if (!ChangingLocationAvailabilityHandle.IsValid()) return;

	// Unregister handler from EventBus and invalidate handle.
	// (Снимаем регистрацию с EventBus и инвалиируем дескриптор.)
	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		EventBus->UnregisterHandler(ChangingLocationAvailabilityHandle);
		UE_LOG(LogTemp, Log, TEXT("WorldStateSubsystem: Unsubscribed ChangingLocationAvailability (handle=%u)"), ChangingLocationAvailabilityHandle.GetId());
	}
	ChangingLocationAvailabilityHandle.Invalidate();
}

void UWorldStateSubsystem::UnsubscribeAll()
{
	// Unsubscribe all related handlers.
	// (Отписать все связанные обработчики.)
	UnsubscribeChangingLocationAvailability();
}

void UWorldStateSubsystem::HandleChangingLocationAvailability(const FOutcomeEventBase& Outcome)
{
	// Defensive check: verify that the incoming outcome satisfies the condition asset.
	// (Защитная проверка: удостовериться, что входящее событие соответствует ассету условия.)
	if (ChangingLocationAvailabilityCondition)
	{
		// Use already compiled query (do not force recompile).
		// (Используем уже скомпилированную Query, не форсируем повторную компиляцию.)
		auto Query = ChangingLocationAvailabilityCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome))
		{
			UE_LOG(LogTemp, Verbose, TEXT("WorldStateSubsystem: Incoming outcome does not satisfy ChangingLocationAvailabilityCondition -> ignoring"));
			return;
		}
	}

	// Try to cast Payload to expected type and log.
	// (Пробуем привести Payload к ожидаемому типу и залогировать.)
	if (UChangingLocationAvailabilityPayload* P = Cast<UChangingLocationAvailabilityPayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("WorldStateSubsystem: Location - Name: %s, IsAvailable: %s"),
			*P->LocationName, P->bIsAvailable ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("WorldStateSubsystem: HandleChangingLocationAvailability called with no payload or unexpected payload type"));
	}

	// Broadcast the unified event to Blueprint listeners.
	// (Шлём унифицированное событие слушателям Blueprint.)
	OnChangingLocationAvailability.Broadcast(Outcome);
}

void UWorldStateSubsystem::HandleSetWorldStateRecord(const FOutcomeEventBase& Outcome)
{
	if (UWorldStateRecordPayload* P = Cast<UWorldStateRecordPayload>(Outcome.Payload))
	{
		SetWorldStateRecord(P->Record);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// WORLD STATE RECORDS
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

bool UWorldStateSubsystem::HasWorldStateRecord(const FGuid& ItemId, FName ChangeKey) const
{
	const TMap<FName, FWorldStateRecord>* Inner = WorldStateRecords.Find(ItemId);
	if (!Inner) return false;
	return Inner->Contains(ChangeKey);
}

bool UWorldStateSubsystem::GetWorldStateRecord(
	const FGuid& ItemId, FName ChangeKey, FWorldStateRecord& OutRecord) const
{
	const TMap<FName, FWorldStateRecord>* Inner = WorldStateRecords.Find(ItemId);
	if (!Inner) return false;
	const FWorldStateRecord* Found = Inner->Find(ChangeKey);
	if (!Found) return false;
	OutRecord = *Found;
	return true;
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

TArray<FWorldStateRecord> UWorldStateSubsystem::GetRecordsByCategory(
	EWorldStateChangeCategory Category) const
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

void UWorldStateSubsystem::ApplyRecordsToWorld()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Собираем карту ItemId -> Actor по UFloorAssignmentComponent
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

	// Ищем свойство с именем ChangeKey на акторе
	FProperty* Prop = FindFProperty<FProperty>(Actor->GetClass(), Record.ChangeKey);
	if (Prop && Prop->HasAllPropertyFlags(CPF_SaveGame))
	{
		Prop->ImportText_InContainer(*Record.SerializedValue, Actor, Actor, PPF_None);
		UE_LOG(LogTemp, Verbose,
			TEXT("WorldStateSubsystem: Applied '%s'='%s' to actor '%s'"),
			*Record.ChangeKey.ToString(), *Record.SerializedValue, *Actor->GetName());
		return;
	}

	// Если свойство не найдено на акторе — ищем в компонентах
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
// ISaveableSubsystem
// ─────────────────────────────────────────────────────────────────────────────

void UWorldStateSubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
	OutData.SubsystemName = GetSaveSubsystemName();

	// Сериализуем все записи в JSON
	TArray<TSharedPtr<FJsonValue>> RecordsArray;

	for (const auto& OuterPair : WorldStateRecords)
	{
		for (const auto& InnerPair : OuterPair.Value)
		{
			const FWorldStateRecord& Rec = InnerPair.Value;
			TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
			Obj->SetStringField(TEXT("ItemId"),       Rec.ItemId.ToString());
			Obj->SetNumberField(TEXT("Category"),     static_cast<int32>(Rec.Category));
			Obj->SetStringField(TEXT("ChangeKey"),    Rec.ChangeKey.ToString());
			Obj->SetStringField(TEXT("Value"),        Rec.SerializedValue);
			Obj->SetStringField(TEXT("MissionId"),    Rec.SourceMissionId.ToString());
			Obj->SetStringField(TEXT("Timestamp"),    Rec.Timestamp);
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

		Obj->TryGetStringField(TEXT("ItemId"),    ItemIdStr);
		Obj->TryGetNumberField(TEXT("Category"),  CategoryInt);
		Obj->TryGetStringField(TEXT("ChangeKey"), ChangeKeyStr);
		Obj->TryGetStringField(TEXT("Value"),     Value);
		Obj->TryGetStringField(TEXT("MissionId"), MissionIdStr);
		Obj->TryGetStringField(TEXT("Timestamp"), Timestamp);

		FGuid ItemId;
		if (!FGuid::Parse(ItemIdStr, ItemId)) continue;

		FWorldStateRecord Record;
		Record.ItemId           = ItemId;
		Record.Category         = static_cast<EWorldStateChangeCategory>(CategoryInt);
		Record.ChangeKey        = FName(*ChangeKeyStr);
		Record.SerializedValue  = Value;
		Record.SourceMissionId  = FName(*MissionIdStr);
		Record.Timestamp        = Timestamp;

		WorldStateRecords.FindOrAdd(ItemId).Add(Record.ChangeKey, Record);
	}

	UE_LOG(LogTemp, Log,
		TEXT("WorldStateSubsystem::ApplySaveData: loaded %d items with records"),
		WorldStateRecords.Num());

	// Сразу применяем к миру если он доступен
	ApplyRecordsToWorld();
}



