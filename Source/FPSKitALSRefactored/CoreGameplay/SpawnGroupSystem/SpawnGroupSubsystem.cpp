// SpawnGroupSubsystem.cpp
#include "SpawnGroupSubsystem.h"
#include "SpawnGroupSpawner.h"
#include "SpawnGroupCommandPayload.h"
#include "SpawnGroupRegistrationPayload.h"
#include "../SaveGame/GameSaveSubsystem.h"
#include "../InteriorInstanceSystem/FloorAssignmentComponent.h"
#include "EngineUtils.h"
#include "JsonObjectConverter.h"
#include <LevelLoadedPayload.h>
#include <Kismet/GameplayStatics.h>
#include <InteriorTransitionPayload.h>
#include "FloorAsset.h"
#include "InteriorSetAsset.h"
#include "GhostCapturedPayload.h"
#include "../LocationSystem/LocationAnchorActor.h"

// ============================================================================
// Вспомогательная функция для подсчёта слотов по предикату
// ============================================================================
static int32 CountSlots(const TArray<FSpawnSlotState>& Slots, TFunction<bool(const FSpawnSlotState&)> Predicate)
{
    int32 Count = 0;
    for (const FSpawnSlotState& Slot : Slots)
        if (Predicate(Slot)) Count++;
    return Count;
}

// ============================================================================
// Жизненный цикл
// ============================================================================

void USpawnGroupSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CachedEventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    SubscribeEvents();

    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
        SaveSys->RegisterSaveableSubsystem(this);
}

void USpawnGroupSubsystem::Deinitialize()
{
    UnsubscribeEvents();
    SpawnerByItemId.Empty();
    GroupIdToItemId.Empty();
    PersistentGroupStates.Empty();
    CachedEventBus.Reset();

    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
        SaveSys->UnregisterSaveableSubsystem(this);

    Super::Deinitialize();
}

// ============================================================================
// Подписка на события EventBus
// ============================================================================

void USpawnGroupSubsystem::SubscribeEvents()
{
    if (!CachedEventBus.IsValid()) return;
    UEventBusSubsystem* EventBus = CachedEventBus.Get();
    if (!EventBus) return;

    auto CreateCondition = [&](EOutcomeSpawnGroup EventType) -> UOutcomeConditionAsset*
        {
            UOutcomeConditionAsset* Asset = NewObject<UOutcomeConditionAsset>(this);
            Asset->OperatorType = EConditionOperator::Composite;
            Asset->FilterRow.OutcomeType = EOutcomeType::SpawnGroup;
            Asset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
            Asset->FilterRow.SpawnGroupType = EventType;
            Asset->FilterRow.SpawnGroupComparison = EConditionComparison::Equals;
            Asset->CompileCondition();
            return Asset;
        };

    auto CreateInteriorCondition = [&](EOutcomeInterior EventType) -> UOutcomeConditionAsset*
        {
            UOutcomeConditionAsset* Asset = NewObject<UOutcomeConditionAsset>(this);
            Asset->OperatorType = EConditionOperator::Composite;
            Asset->FilterRow.OutcomeType = EOutcomeType::Interior;
            Asset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
            Asset->FilterRow.InteriorType = EventType;
            Asset->FilterRow.InteriorComparison = EConditionComparison::Equals;
            Asset->CompileCondition();
            return Asset;
        };

    if (!RegisterHandle.IsValid())
    {
        UOutcomeConditionAsset* Cond = CreateCondition(EOutcomeSpawnGroup::SpawnGroupRegister);
        if (Cond->GetCondition().IsValid())
            RegisterHandle = EventBus->RegisterHandler(Cond, FOutcomeHandlerDelegate::CreateUObject(this, &USpawnGroupSubsystem::HandleSpawnGroupRegister));
    }
    if (!UnregisterHandle.IsValid())
    {
        UOutcomeConditionAsset* Cond = CreateCondition(EOutcomeSpawnGroup::SpawnGroupUnregister);
        if (Cond->GetCondition().IsValid())
            UnregisterHandle = EventBus->RegisterHandler(Cond, FOutcomeHandlerDelegate::CreateUObject(this, &USpawnGroupSubsystem::HandleSpawnGroupUnregister));
    }
    if (!ActivateHandle.IsValid())
    {
        UOutcomeConditionAsset* Cond = CreateCondition(EOutcomeSpawnGroup::SpawnGroupActivated);
        if (Cond->GetCondition().IsValid())
            ActivateHandle = EventBus->RegisterHandler(Cond, FOutcomeHandlerDelegate::CreateUObject(this, &USpawnGroupSubsystem::HandleSpawnGroupActivated));
    }
    if (!ClearHandle.IsValid())
    {
        UOutcomeConditionAsset* Cond = CreateCondition(EOutcomeSpawnGroup::SpawnGroupCleared);
        if (Cond->GetCondition().IsValid())
            ClearHandle = EventBus->RegisterHandler(Cond, FOutcomeHandlerDelegate::CreateUObject(this, &USpawnGroupSubsystem::HandleSpawnGroupCleared));
    }
    if (!ResetHandle.IsValid())
    {
        UOutcomeConditionAsset* Cond = CreateCondition(EOutcomeSpawnGroup::SpawnGroupReset);
        if (Cond->GetCondition().IsValid())
            ResetHandle = EventBus->RegisterHandler(Cond, FOutcomeHandlerDelegate::CreateUObject(this, &USpawnGroupSubsystem::HandleSpawnGroupReset));
    }
    if (!LevelLoadedHandle.IsValid())
    {
        UOutcomeConditionAsset* Cond = CreateInteriorCondition(EOutcomeInterior::LevelLoaded);
        if (Cond->GetCondition().IsValid())
            LevelLoadedHandle = EventBus->RegisterHandler(Cond, FOutcomeHandlerDelegate::CreateUObject(this, &USpawnGroupSubsystem::HandleLevelLoaded));
    }
    if (!FloorLeavingHandle.IsValid())
    {
        UOutcomeConditionAsset* Cond = CreateInteriorCondition(EOutcomeInterior::FloorLeaving);
        if (Cond->GetCondition().IsValid())
            FloorLeavingHandle = EventBus->RegisterHandler(Cond, FOutcomeHandlerDelegate::CreateUObject(this, &USpawnGroupSubsystem::HandleFloorLeaving));
    }

    if (!GhostCapturedHandle.IsValid())
    {
        UOutcomeConditionAsset* Cond = CreateCondition(EOutcomeSpawnGroup::GhostCaptured);
        if (Cond->GetCondition().IsValid())
            GhostCapturedHandle = EventBus->RegisterHandler(Cond, FOutcomeHandlerDelegate::CreateUObject(this, &USpawnGroupSubsystem::HandleGhostCaptured));
    }
}

void USpawnGroupSubsystem::UnsubscribeEvents()
{
    if (!CachedEventBus.IsValid()) return;
    UEventBusSubsystem* EventBus = CachedEventBus.Get();
    if (!EventBus) return;

    auto Unreg = [&](FOutcomeHandlerHandle& Handle)
        {
            if (Handle.IsValid())
            {
                EventBus->UnregisterHandler(Handle);
                Handle.Invalidate();
            }
        };
    Unreg(RegisterHandle);
    Unreg(UnregisterHandle);
    Unreg(ActivateHandle);
    Unreg(ClearHandle);
    Unreg(ResetHandle);
    Unreg(GhostCapturedHandle);
}

// ============================================================================
// Обработчики событий
// ============================================================================

void USpawnGroupSubsystem::HandleSpawnGroupRegister(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeType == EOutcomeType::SpawnGroup &&
        Outcome.OutcomeSpawnGroup == EOutcomeSpawnGroup::SpawnGroupRegister)
    {
        USpawnGroupRegistrationPayload* P = Cast<USpawnGroupRegistrationPayload>(Outcome.Payload);
        if (!P) return;

        ASpawnGroupSpawner* Spawner = nullptr;
        for (TActorIterator<ASpawnGroupSpawner> It(GetWorld()); It; ++It)
        {
            if (UFloorAssignmentComponent* Comp = It->FindComponentByClass<UFloorAssignmentComponent>())
            {
                if (Comp->ItemId == P->SpawnerId)
                {
                    Spawner = *It;
                    break;
                }
            }
        }
        if (Spawner)
        {
            SpawnerByItemId.Add(P->SpawnerId, Spawner);
            GroupIdToItemId.Add(P->GroupId, P->SpawnerId);
            UE_LOG(LogTemp, Log, TEXT("SpawnGroupSubsystem: Registered spawner for group %s"), *P->GroupId.ToString());
        }
    }
}

void USpawnGroupSubsystem::HandleSpawnGroupUnregister(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeType == EOutcomeType::SpawnGroup &&
        Outcome.OutcomeSpawnGroup == EOutcomeSpawnGroup::SpawnGroupUnregister)
    {
        USpawnGroupRegistrationPayload* P = Cast<USpawnGroupRegistrationPayload>(Outcome.Payload);
        if (!P) return;
        SpawnerByItemId.Remove(P->SpawnerId);
        for (auto It = GroupIdToItemId.CreateIterator(); It; ++It)
        {
            if (It->Value == P->SpawnerId)
            {
                It.RemoveCurrent();
                break;
            }
        }
    }
}

void USpawnGroupSubsystem::HandleSpawnGroupActivated(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeType == EOutcomeType::SpawnGroup &&
        Outcome.OutcomeSpawnGroup == EOutcomeSpawnGroup::SpawnGroupActivated)
    {
        USpawnGroupCommandPayload* P = Cast<USpawnGroupCommandPayload>(Outcome.Payload);
        if (!P) return;
        ActivateSpawnGroupInternal(P->GroupId);
    }
}

void USpawnGroupSubsystem::HandleSpawnGroupCleared(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeType == EOutcomeType::SpawnGroup &&
        Outcome.OutcomeSpawnGroup == EOutcomeSpawnGroup::SpawnGroupCleared)
    {
        USpawnGroupCommandPayload* P = Cast<USpawnGroupCommandPayload>(Outcome.Payload);
        if (!P) return;
        ClearSpawnGroupInternal(P->GroupId, P->Reason);
    }
}

void USpawnGroupSubsystem::HandleSpawnGroupReset(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeType == EOutcomeType::SpawnGroup &&
        Outcome.OutcomeSpawnGroup == EOutcomeSpawnGroup::SpawnGroupReset)
    {
        USpawnGroupCommandPayload* P = Cast<USpawnGroupCommandPayload>(Outcome.Payload);
        if (!P) return;
        ResetSpawnGroupInternal(P->GroupId);
    }
}

void USpawnGroupSubsystem::HandleFloorLeaving(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeType != EOutcomeType::Interior ||
        Outcome.OutcomeInterior != EOutcomeInterior::FloorLeaving)
        return;

    UInteriorTransitionPayload* Payload = Cast<UInteriorTransitionPayload>(Outcome.Payload);
    if (!Payload) return;

    UFloorAsset* SourceFloorAsset = Payload->SourceFloor.LoadSynchronous();
    if (!SourceFloorAsset) return;

    FGuid FloorId = SourceFloorAsset->FloorID;
    FGuid InteriorSetId = SourceFloorAsset->InteriorSetID;
    if (!InteriorSetId.IsValid())
    {
        if (UInteriorSetAsset* Parent = SourceFloorAsset->ParentInteriorSet.LoadSynchronous())
            InteriorSetId = Parent->InteriorSetID;
    }
    if (!FloorId.IsValid() || !InteriorSetId.IsValid()) return;

    FInteriorFloorKey LeavingKey(InteriorSetId, FloorId);

    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;

        FInteriorFloorKey SpawnerKey = GetFloorKeyFromSpawner(Spawner);
        if (SpawnerKey == LeavingKey)
        {
            UpdateSpawnerStateInCache(Spawner);
        }
    }
}

void USpawnGroupSubsystem::HandleGhostCaptured(const FOutcomeEventBase& Outcome)
{
    UGhostCapturedPayload* P = Cast<UGhostCapturedPayload>(Outcome.Payload);
    if (!P) return;
    AActor* Actor = P->CapturedActor.Get();
    if (!Actor) return;

    FGuid ItemId = P->ItemId;
    if (!ItemId.IsValid() && Actor)
    {
        if (UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>())
            ItemId = Comp->ItemId;
    }
    if (!ItemId.IsValid()) return;

    for (auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner) continue;
        for (FSpawnSlotState& Slot : Spawner->GetSlotsMutable())
        {
            if (Slot.ItemId == ItemId)
            {
                Slot.State = EGhostState::Captured;
                return;
            }
        }
    }
}

// ============================================================================
// Внутренние методы управления
// ============================================================================

void USpawnGroupSubsystem::ActivateSpawnGroupInternal(const FGuid& GroupId)
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(GroupId);
    if (Spawner)
        Spawner->SpawnGroupInternal();
}

void USpawnGroupSubsystem::ClearSpawnGroupInternal(const FGuid& GroupId, ESpawnGroupResolutionReason Reason)
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(GroupId);
    if (Spawner)
        Spawner->ClearGroup(Reason);
}

void USpawnGroupSubsystem::ResetSpawnGroupInternal(const FGuid& GroupId)
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(GroupId);
    if (Spawner)
        Spawner->ResetGroup();
}

void USpawnGroupSubsystem::UpdateSpawnerStateInCache(ASpawnGroupSpawner* Spawner)
{
    if (!Spawner || !IsValid(Spawner)) return;

    FInteriorFloorKey FloorKey = GetFloorKeyFromSpawner(Spawner);
    if (!FloorKey.FloorId.IsValid()) return;

    FSpawnGroupState State;
    State.GroupId = Spawner->GetRuntimeGroupId();
    State.Status = Spawner->GetCurrentStatus();
    State.bStoreSpawnParameters = Spawner->IsStoreSpawnParameters;
    State.KilledCount = Spawner->GetKilledCount();

    // Всегда сохраняем слоты (даже если IsStoreSpawnParameters == false)
    State.Slots = Spawner->GetAllSlots();

    // Сохраняем TypeKilled для обратной совместимости (если нужно)
    State.TypeKilled = Spawner->GetTypeKilled();

    TMap<FGuid, FSpawnGroupState>& FloorStates = PersistentGroupStates.FindOrAdd(FloorKey);
    FloorStates.Add(Spawner->GetRuntimeGroupId(), State);
}

// ============================================================================
// Поиск спавнеров
// ============================================================================

ASpawnGroupSpawner* USpawnGroupSubsystem::FindSpawnerByItemId(const FGuid& ItemId) const
{
    const TWeakObjectPtr<ASpawnGroupSpawner>* Found = SpawnerByItemId.Find(ItemId);
    return (Found && Found->IsValid()) ? Found->Get() : nullptr;
}

ASpawnGroupSpawner* USpawnGroupSubsystem::FindSpawnerByGroupId(const FGuid& GroupId) const
{
    const FGuid* ItemId = GroupIdToItemId.Find(GroupId);
    return ItemId ? FindSpawnerByItemId(*ItemId) : nullptr;
}

// ============================================================================
// Обработчик загрузки уровня
// ============================================================================

void USpawnGroupSubsystem::HandleLevelLoaded(const FOutcomeEventBase& Outcome)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (Outcome.OutcomeType != EOutcomeType::Interior ||
        Outcome.OutcomeInterior != EOutcomeInterior::LevelLoaded)
        return;

    // Определяем текущий ключ этажа
    ALocationAnchorActor* FoundAnchor = nullptr;
    for (TActorIterator<ALocationAnchorActor> It(World); It; ++It)
    {
        FoundAnchor = *It;
        break;
    }
    if (FoundAnchor)
    {
        UFloorAsset* FloorAsset = FoundAnchor->OwnerFloor.LoadSynchronous();
        UInteriorSetAsset* InteriorAsset = FoundAnchor->OwnerInteriorSet.LoadSynchronous();
        if (InteriorAsset && InteriorAsset->InteriorSetID.IsValid() && FloorAsset && FloorAsset->FloorID.IsValid())
        {
            CurrentFloorKey = FInteriorFloorKey(InteriorAsset->InteriorSetID, FloorAsset->FloorID);
        }
    }

    // Восстанавливаем состояние спавнеров из PersistentGroupStates
    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;

        if (Spawner->Restore || !Spawner->SpawnGroupAsset || Spawner->CurrentStatus == ESpawnGroupStatus::Suppressed || Spawner->CurrentStatus == ESpawnGroupStatus::Cleared || Spawner->CurrentStatus == ESpawnGroupStatus::Inactive)
            continue;

        FInteriorFloorKey FloorKey = GetFloorKeyFromSpawner(Spawner);
        if (!FloorKey.FloorId.IsValid()) continue;

        TMap<FGuid, FSpawnGroupState>* FloorStates = PersistentGroupStates.Find(FloorKey);
        if (!FloorStates) continue;

        FSpawnGroupState* State = FloorStates->Find(Spawner->GetRuntimeGroupId());
        if (!State) continue;

        // Восстанавливаем флаг спавнера (если изменился в рантайме)
        Spawner->IsStoreSpawnParameters = State->bStoreSpawnParameters;

        // Восстанавливаем призраков без изменения AllSlots и без уничтожения существующих
        Spawner->RestoreFromStateWithoutCleanup(*State);
    }
}

// ============================================================================
// Сохранение и загрузка
// ============================================================================

void USpawnGroupSubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
    OutData.SubsystemName = GetSaveSubsystemName();
    PersistentGroupStates.Empty();

    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;

        FInteriorFloorKey FloorKey = GetFloorKeyFromSpawner(Spawner);
        if (!FloorKey.FloorId.IsValid()) continue;

        FSpawnGroupState State;
        State.GroupId = Spawner->GetRuntimeGroupId();
        State.Status = Spawner->GetCurrentStatus();
        State.bStoreSpawnParameters = Spawner->IsStoreSpawnParameters;
        State.KilledCount = Spawner->GetKilledCount();

        // Всегда сохраняем слоты
        State.Slots = Spawner->GetAllSlots();
        State.TypeKilled = Spawner->GetTypeKilled();

        TMap<FGuid, FSpawnGroupState>& FloorStates = PersistentGroupStates.FindOrAdd(FloorKey);
        FloorStates.Add(Spawner->GetRuntimeGroupId(), State);
    }

    // Сериализация в JSON
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    TSharedPtr<FJsonObject> GroupsObj = MakeShared<FJsonObject>();

    for (const auto& FloorPair : PersistentGroupStates)
    {
        FString KeyStr = FString::Printf(TEXT("%s|%s"), *FloorPair.Key.InteriorSetId.ToString(), *FloorPair.Key.FloorId.ToString());
        TSharedPtr<FJsonObject> FloorGroups = MakeShared<FJsonObject>();

        for (const auto& GroupPair : FloorPair.Value)
        {
            TSharedPtr<FJsonObject> StateObj = MakeShared<FJsonObject>();
            StateObj->SetStringField(TEXT("GroupId"), GroupPair.Key.ToString());
            StateObj->SetNumberField(TEXT("Status"), static_cast<uint8>(GroupPair.Value.Status));
            StateObj->SetNumberField(TEXT("ResolutionReason"), static_cast<uint8>(GroupPair.Value.ResolutionReason));
            StateObj->SetBoolField(TEXT("bStoreSpawnParameters"), GroupPair.Value.bStoreSpawnParameters);
            StateObj->SetNumberField(TEXT("KilledCount"), GroupPair.Value.KilledCount);

            // ---- Всегда сериализуем Slots ----
            TArray<TSharedPtr<FJsonValue>> SlotsArray;
            for (const FSpawnSlotState& Slot : GroupPair.Value.Slots)
            {
                TSharedPtr<FJsonObject> SlotObj = MakeShared<FJsonObject>();
                SlotObj->SetStringField(TEXT("ItemId"), Slot.ItemId.ToString());
                SlotObj->SetStringField(TEXT("ActorClass"), Slot.ActorClass ? Slot.ActorClass->GetPathName() : TEXT(""));
                FVector Loc = Slot.SpawnTransform.GetLocation();
                FRotator Rot = Slot.SpawnTransform.Rotator();
                SlotObj->SetArrayField(TEXT("Location"), { MakeShared<FJsonValueNumber>(Loc.X), MakeShared<FJsonValueNumber>(Loc.Y), MakeShared<FJsonValueNumber>(Loc.Z) });
                SlotObj->SetArrayField(TEXT("Rotation"), { MakeShared<FJsonValueNumber>(Rot.Pitch), MakeShared<FJsonValueNumber>(Rot.Yaw), MakeShared<FJsonValueNumber>(Rot.Roll) });
                SlotObj->SetNumberField(TEXT("State"), static_cast<uint8>(Slot.State));

                // GameplayTags
                TArray<TSharedPtr<FJsonValue>> GameplayTagArray;
                for (const FGameplayTag& Tag : Slot.GameplayTags)
                    GameplayTagArray.Add(MakeShared<FJsonValueString>(Tag.ToString()));
                SlotObj->SetArrayField(TEXT("GameplayTags"), GameplayTagArray);

                // TextTags
                TArray<TSharedPtr<FJsonValue>> TextTagArray;
                for (const FName& Tag : Slot.TextTags)
                    TextTagArray.Add(MakeShared<FJsonValueString>(Tag.ToString()));
                SlotObj->SetArrayField(TEXT("TextTags"), TextTagArray);

                SlotsArray.Add(MakeShared<FJsonValueObject>(SlotObj));
            }
            StateObj->SetArrayField(TEXT("Slots"), SlotsArray);

            // ---- Сериализуем TypeKilled для обратной совместимости ----
            if (GroupPair.Value.TypeKilled.Num() > 0)
            {
                TSharedPtr<FJsonObject> TypeKilledObj = MakeShared<FJsonObject>();
                for (const auto& TypePair : GroupPair.Value.TypeKilled)
                    TypeKilledObj->SetNumberField(TypePair.Key.ToString(), TypePair.Value);
                StateObj->SetObjectField(TEXT("TypeKilled"), TypeKilledObj);
            }

            FloorGroups->SetObjectField(GroupPair.Key.ToString(), StateObj);
        }
        GroupsObj->SetObjectField(KeyStr, FloorGroups);
    }

    Root->SetObjectField(TEXT("PersistentGroupStates"), GroupsObj);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    OutData.SerializedData = Output;
}

void USpawnGroupSubsystem::ApplySaveData(const FSubsystemSaveData& InData)
{
    bIsLoadComplete = false;
    if (InData.SerializedData.IsEmpty())
    {
        bIsLoadComplete = true;
        return;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InData.SerializedData);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        bIsLoadComplete = true;
        return;
    }

    PersistentGroupStates.Empty();

    const TSharedPtr<FJsonObject>* GroupsObjPtr = nullptr;
    if (Root->TryGetObjectField(TEXT("PersistentGroupStates"), GroupsObjPtr))
    {
        for (const auto& FloorPair : (*GroupsObjPtr)->Values)
        {
            TArray<FString> Parts;
            FloorPair.Key.ParseIntoArray(Parts, TEXT("|"), false);
            if (Parts.Num() != 2) continue;
            FGuid InteriorSetId, FloorId;
            if (!FGuid::Parse(Parts[0], InteriorSetId) || !FGuid::Parse(Parts[1], FloorId)) continue;
            FInteriorFloorKey Key(InteriorSetId, FloorId);

            TMap<FGuid, FSpawnGroupState> FloorGroups;
            const TSharedPtr<FJsonObject>* FloorGroupsObj = nullptr;
            if (FloorPair.Value->TryGetObject(FloorGroupsObj))
            {
                for (const auto& GroupPair : (*FloorGroupsObj)->Values)
                {
                    FGuid GroupGuid;
                    if (!FGuid::Parse(GroupPair.Key, GroupGuid)) continue;
                    FGuid GroupId(GroupGuid);

                    const TSharedPtr<FJsonObject>* StateObj = nullptr;
                    if (!GroupPair.Value->TryGetObject(StateObj)) continue;

                    FSpawnGroupState State;
                    State.GroupId = GroupId;

                    if ((*StateObj)->HasField(TEXT("Status")))
                        State.Status = static_cast<ESpawnGroupStatus>((*StateObj)->GetIntegerField(TEXT("Status")));
                    if ((*StateObj)->HasField(TEXT("ResolutionReason")))
                        State.ResolutionReason = static_cast<ESpawnGroupResolutionReason>((*StateObj)->GetIntegerField(TEXT("ResolutionReason")));
                    if ((*StateObj)->HasField(TEXT("KilledCount")))
                        State.KilledCount = (*StateObj)->GetIntegerField(TEXT("KilledCount"));
                    if ((*StateObj)->HasField(TEXT("bStoreSpawnParameters")))
                        State.bStoreSpawnParameters = (*StateObj)->GetBoolField(TEXT("bStoreSpawnParameters"));
                    else
                        State.bStoreSpawnParameters = false;

                    // ---- Десериализуем Slots (всегда) ----
                    const TArray<TSharedPtr<FJsonValue>>* SlotsArray = nullptr;
                    if ((*StateObj)->TryGetArrayField(TEXT("Slots"), SlotsArray))
                    {
                        for (const TSharedPtr<FJsonValue>& SlotVal : *SlotsArray)
                        {
                            const TSharedPtr<FJsonObject>* SlotObj = nullptr;
                            if (!SlotVal->TryGetObject(SlotObj)) continue;

                            FSpawnSlotState Slot;
                            FString ItemIdStr;
                            if ((*SlotObj)->TryGetStringField(TEXT("ItemId"), ItemIdStr))
                                FGuid::Parse(ItemIdStr, Slot.ItemId);

                            FString ClassPath;
                            if ((*SlotObj)->TryGetStringField(TEXT("ActorClass"), ClassPath) && !ClassPath.IsEmpty())
                                Slot.ActorClass = LoadClass<AActor>(nullptr, *ClassPath);

                            const TArray<TSharedPtr<FJsonValue>>* LocArr = nullptr;
                            if ((*SlotObj)->TryGetArrayField(TEXT("Location"), LocArr) && LocArr->Num() >= 3)
                            {
                                FVector Loc((*LocArr)[0]->AsNumber(), (*LocArr)[1]->AsNumber(), (*LocArr)[2]->AsNumber());
                                Slot.SpawnTransform.SetLocation(Loc);
                            }
                            const TArray<TSharedPtr<FJsonValue>>* RotArr = nullptr;
                            if ((*SlotObj)->TryGetArrayField(TEXT("Rotation"), RotArr) && RotArr->Num() >= 3)
                            {
                                FRotator Rot((*RotArr)[0]->AsNumber(), (*RotArr)[1]->AsNumber(), (*RotArr)[2]->AsNumber());
                                Slot.SpawnTransform.SetRotation(Rot.Quaternion());
                            }

                            uint8 StateValue = 0;
                            if ((*SlotObj)->TryGetNumberField(TEXT("State"), StateValue))
                                Slot.State = static_cast<EGhostState>(StateValue);
                            else
                                Slot.State = EGhostState::Alive;

                            const TArray<TSharedPtr<FJsonValue>>* GameplayTagArray = nullptr;
                            if ((*SlotObj)->TryGetArrayField(TEXT("GameplayTags"), GameplayTagArray))
                            {
                                for (const auto& Val : *GameplayTagArray)
                                {
                                    if (Val->Type == EJson::String)
                                    {
                                        FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Val->AsString()));
                                        if (Tag.IsValid())
                                            Slot.GameplayTags.AddTag(Tag);
                                    }
                                }
                            }

                            const TArray<TSharedPtr<FJsonValue>>* TextTagArray = nullptr;
                            if ((*SlotObj)->TryGetArrayField(TEXT("TextTags"), TextTagArray))
                            {
                                for (const auto& Val : *TextTagArray)
                                {
                                    if (Val->Type == EJson::String)
                                        Slot.TextTags.Add(FName(*Val->AsString()));
                                }
                            }

                            State.Slots.Add(Slot);
                        }
                    }

                    // ---- Десериализуем TypeKilled (если есть) ----
                    const TSharedPtr<FJsonObject>* TypeKilledObj = nullptr;
                    if ((*StateObj)->TryGetObjectField(TEXT("TypeKilled"), TypeKilledObj))
                    {
                        for (const auto& TypePair : (*TypeKilledObj)->Values)
                        {
                            FName ClassName(*TypePair.Key);
                            int32 Count = static_cast<int32>(TypePair.Value->AsNumber());
                            State.TypeKilled.Add(ClassName, Count);
                        }
                    }

                    FloorGroups.Add(GroupId, State);
                }
            }
            PersistentGroupStates.Add(Key, FloorGroups);
        }
    }

    bIsLoadComplete = true;
}

// ============================================================================
// Вспомогательные методы
// ============================================================================

FInteriorFloorKey USpawnGroupSubsystem::GetFloorKeyFromSpawner(ASpawnGroupSpawner* Spawner) const
{
    if (!Spawner) return FInteriorFloorKey();
    UFloorAssignmentComponent* Comp = Spawner->FindComponentByClass<UFloorAssignmentComponent>();
    if (!Comp) return FInteriorFloorKey();
    return FInteriorFloorKey(Comp->InteriorSetId, Comp->FloorId);
}

// ============================================================================
// Методы статистики по группе
// ============================================================================

int32 USpawnGroupSubsystem::GetTotalSpawnedCount(const FGuid& ItemId) const
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(ItemId);
    if (Spawner)
        return Spawner->GetAllSlots().Num();
    return 0;
}

int32 USpawnGroupSubsystem::GetAliveCount(const FGuid& ItemId) const
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(ItemId);
    if (Spawner)
        return CountSlots(Spawner->GetAllSlots(), [](const FSpawnSlotState& Slot) { return Slot.State == EGhostState::Alive; });
    return 0;
}

int32 USpawnGroupSubsystem::GetKilledCount(const FGuid& ItemId) const
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(ItemId);
    if (Spawner)
        return CountSlots(Spawner->GetAllSlots(), [](const FSpawnSlotState& Slot) { return Slot.State == EGhostState::Killed; });
    return 0;
}

int32 USpawnGroupSubsystem::GetCapturedCount(const FGuid& ItemId) const
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(ItemId);
    if (Spawner)
        return CountSlots(Spawner->GetAllSlots(), [](const FSpawnSlotState& Slot) { return Slot.State == EGhostState::Captured; });
    return 0;
}

int32 USpawnGroupSubsystem::GetAliveCountByType(const FGuid& ItemId, TSubclassOf<AActor> ActorClass) const
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(ItemId);
    if (Spawner)
        return CountSlots(Spawner->GetAllSlots(), [&](const FSpawnSlotState& Slot) {
        return Slot.State == EGhostState::Alive && Slot.ActorClass == ActorClass;
            });
    return 0;
}

int32 USpawnGroupSubsystem::GetKilledCountByType(const FGuid& ItemId, TSubclassOf<AActor> ActorClass) const
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(ItemId);
    if (Spawner)
        return CountSlots(Spawner->GetAllSlots(), [&](const FSpawnSlotState& Slot) {
        return Slot.State == EGhostState::Killed && Slot.ActorClass == ActorClass;
            });
    return 0;
}

int32 USpawnGroupSubsystem::GetCapturedCountByType(const FGuid& ItemId, TSubclassOf<AActor> ActorClass) const
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(ItemId);
    if (Spawner)
        return CountSlots(Spawner->GetAllSlots(), [&](const FSpawnSlotState& Slot) {
        return Slot.State == EGhostState::Captured && Slot.ActorClass == ActorClass;
            });
    return 0;
}

int32 USpawnGroupSubsystem::GetAliveCountByTextTag(const FGuid& ItemId, const TArray<FName>& TextTags) const
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(ItemId);
    if (!Spawner) return 0;
    int32 Count = 0;
    for (const FSpawnSlotState& Slot : Spawner->GetAllSlots())
    {
        if (Slot.State == EGhostState::Alive)
        {
            for (const FName& Tag : TextTags)
                if (Slot.TextTags.Contains(Tag)) { Count++; break; }
        }
    }
    return Count;
}

int32 USpawnGroupSubsystem::GetKilledCountByTextTag(const FGuid& ItemId, const TArray<FName>& TextTags) const
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(ItemId);
    if (!Spawner) return 0;
    int32 Count = 0;
    for (const FSpawnSlotState& Slot : Spawner->GetAllSlots())
    {
        if (Slot.State == EGhostState::Killed)
        {
            for (const FName& Tag : TextTags)
                if (Slot.TextTags.Contains(Tag)) { Count++; break; }
        }
    }
    return Count;
}

int32 USpawnGroupSubsystem::GetCapturedCountByTextTag(const FGuid& ItemId, const TArray<FName>& TextTags) const
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(ItemId);
    if (!Spawner) return 0;
    int32 Count = 0;
    for (const FSpawnSlotState& Slot : Spawner->GetAllSlots())
    {
        if (Slot.State == EGhostState::Captured)
        {
            for (const FName& Tag : TextTags)
                if (Slot.TextTags.Contains(Tag)) { Count++; break; }
        }
    }
    return Count;
}

int32 USpawnGroupSubsystem::GetAliveCountByGameplayTag(const FGuid& ItemId, const TArray<FGameplayTag>& GameplayTags) const
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(ItemId);
    if (!Spawner) return 0;
    int32 Count = 0;
    for (const FSpawnSlotState& Slot : Spawner->GetAllSlots())
    {
        if (Slot.State == EGhostState::Alive)
        {
            for (const FGameplayTag& Tag : GameplayTags)
                if (Slot.GameplayTags.HasTag(Tag)) { Count++; break; }
        }
    }
    return Count;
}

int32 USpawnGroupSubsystem::GetKilledCountByGameplayTag(const FGuid& ItemId, const TArray<FGameplayTag>& GameplayTags) const
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(ItemId);
    if (!Spawner) return 0;
    int32 Count = 0;
    for (const FSpawnSlotState& Slot : Spawner->GetAllSlots())
    {
        if (Slot.State == EGhostState::Killed)
        {
            for (const FGameplayTag& Tag : GameplayTags)
                if (Slot.GameplayTags.HasTag(Tag)) { Count++; break; }
        }
    }
    return Count;
}

int32 USpawnGroupSubsystem::GetCapturedCountByGameplayTag(const FGuid& ItemId, const TArray<FGameplayTag>& GameplayTags) const
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(ItemId);
    if (!Spawner) return 0;
    int32 Count = 0;
    for (const FSpawnSlotState& Slot : Spawner->GetAllSlots())
    {
        if (Slot.State == EGhostState::Captured)
        {
            for (const FGameplayTag& Tag : GameplayTags)
                if (Slot.GameplayTags.HasTag(Tag)) { Count++; break; }
        }
    }
    return Count;
}

ESpawnGroupStatus USpawnGroupSubsystem::GetGroupStatus(const FGuid& ItemId) const
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(ItemId);
    if (Spawner)
        return Spawner->GetCurrentStatus();
    return ESpawnGroupStatus::Inactive;
}

// ============================================================================
// Методы статистики по текущему этажу (агрегированные)
// ============================================================================

int32 USpawnGroupSubsystem::GetTotalSpawnedCountForCurrentFloor() const
{
    int32 Total = 0;
    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;
        if (GetFloorKeyFromSpawner(Spawner) == CurrentFloorKey)
            Total += Spawner->GetAllSlots().Num();
    }
    return Total;
}

int32 USpawnGroupSubsystem::GetAliveCountForCurrentFloor() const
{
    int32 Total = 0;
    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;
        if (GetFloorKeyFromSpawner(Spawner) == CurrentFloorKey)
            Total += CountSlots(Spawner->GetAllSlots(), [](const FSpawnSlotState& Slot) { return Slot.State == EGhostState::Alive; });
    }
    return Total;
}

int32 USpawnGroupSubsystem::GetKilledCountForCurrentFloor() const
{
    int32 Total = 0;
    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;
        if (GetFloorKeyFromSpawner(Spawner) == CurrentFloorKey)
            Total += CountSlots(Spawner->GetAllSlots(), [](const FSpawnSlotState& Slot) { return Slot.State == EGhostState::Killed; });
    }
    return Total;
}

int32 USpawnGroupSubsystem::GetCapturedCountForCurrentFloor() const
{
    int32 Total = 0;
    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;
        if (GetFloorKeyFromSpawner(Spawner) == CurrentFloorKey)
            Total += CountSlots(Spawner->GetAllSlots(), [](const FSpawnSlotState& Slot) { return Slot.State == EGhostState::Captured; });
    }
    return Total;
}

int32 USpawnGroupSubsystem::GetAliveCountByTypeForCurrentFloor(TSubclassOf<AActor> ActorClass) const
{
    int32 Total = 0;
    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;
        if (GetFloorKeyFromSpawner(Spawner) == CurrentFloorKey)
            Total += CountSlots(Spawner->GetAllSlots(), [&](const FSpawnSlotState& Slot) {
            return Slot.State == EGhostState::Alive && Slot.ActorClass == ActorClass;
                });
    }
    return Total;
}

int32 USpawnGroupSubsystem::GetKilledCountByTypeForCurrentFloor(TSubclassOf<AActor> ActorClass) const
{
    int32 Total = 0;
    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;
        if (GetFloorKeyFromSpawner(Spawner) == CurrentFloorKey)
            Total += CountSlots(Spawner->GetAllSlots(), [&](const FSpawnSlotState& Slot) {
            return Slot.State == EGhostState::Killed && Slot.ActorClass == ActorClass;
                });
    }
    return Total;
}

int32 USpawnGroupSubsystem::GetCapturedCountByTypeForCurrentFloor(TSubclassOf<AActor> ActorClass) const
{
    int32 Total = 0;
    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;
        if (GetFloorKeyFromSpawner(Spawner) == CurrentFloorKey)
            Total += CountSlots(Spawner->GetAllSlots(), [&](const FSpawnSlotState& Slot) {
            return Slot.State == EGhostState::Captured && Slot.ActorClass == ActorClass;
                });
    }
    return Total;
}

int32 USpawnGroupSubsystem::GetAliveCountByTextTagForCurrentFloor(const TArray<FName>& TextTags) const
{
    int32 Total = 0;
    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;
        if (GetFloorKeyFromSpawner(Spawner) == CurrentFloorKey)
        {
            for (const FSpawnSlotState& Slot : Spawner->GetAllSlots())
            {
                if (Slot.State == EGhostState::Alive)
                {
                    for (const FName& Tag : TextTags)
                        if (Slot.TextTags.Contains(Tag)) { Total++; break; }
                }
            }
        }
    }
    return Total;
}

int32 USpawnGroupSubsystem::GetKilledCountByTextTagForCurrentFloor(const TArray<FName>& TextTags) const
{
    int32 Total = 0;
    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;
        if (GetFloorKeyFromSpawner(Spawner) == CurrentFloorKey)
        {
            for (const FSpawnSlotState& Slot : Spawner->GetAllSlots())
            {
                if (Slot.State == EGhostState::Killed)
                {
                    for (const FName& Tag : TextTags)
                        if (Slot.TextTags.Contains(Tag)) { Total++; break; }
                }
            }
        }
    }
    return Total;
}

int32 USpawnGroupSubsystem::GetCapturedCountByTextTagForCurrentFloor(const TArray<FName>& TextTags) const
{
    int32 Total = 0;
    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;
        if (GetFloorKeyFromSpawner(Spawner) == CurrentFloorKey)
        {
            for (const FSpawnSlotState& Slot : Spawner->GetAllSlots())
            {
                if (Slot.State == EGhostState::Captured)
                {
                    for (const FName& Tag : TextTags)
                        if (Slot.TextTags.Contains(Tag)) { Total++; break; }
                }
            }
        }
    }
    return Total;
}

int32 USpawnGroupSubsystem::GetAliveCountByGameplayTagForCurrentFloor(const TArray<FGameplayTag>& GameplayTags) const
{
    int32 Total = 0;
    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;
        if (GetFloorKeyFromSpawner(Spawner) == CurrentFloorKey)
        {
            for (const FSpawnSlotState& Slot : Spawner->GetAllSlots())
            {
                if (Slot.State == EGhostState::Alive)
                {
                    for (const FGameplayTag& Tag : GameplayTags)
                        if (Slot.GameplayTags.HasTag(Tag)) { Total++; break; }
                }
            }
        }
    }
    return Total;
}

int32 USpawnGroupSubsystem::GetKilledCountByGameplayTagForCurrentFloor(const TArray<FGameplayTag>& GameplayTags) const
{
    int32 Total = 0;
    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;
        if (GetFloorKeyFromSpawner(Spawner) == CurrentFloorKey)
        {
            for (const FSpawnSlotState& Slot : Spawner->GetAllSlots())
            {
                if (Slot.State == EGhostState::Killed)
                {
                    for (const FGameplayTag& Tag : GameplayTags)
                        if (Slot.GameplayTags.HasTag(Tag)) { Total++; break; }
                }
            }
        }
    }
    return Total;
}

int32 USpawnGroupSubsystem::GetCapturedCountByGameplayTagForCurrentFloor(const TArray<FGameplayTag>& GameplayTags) const
{
    int32 Total = 0;
    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;
        if (GetFloorKeyFromSpawner(Spawner) == CurrentFloorKey)
        {
            for (const FSpawnSlotState& Slot : Spawner->GetAllSlots())
            {
                if (Slot.State == EGhostState::Captured)
                {
                    for (const FGameplayTag& Tag : GameplayTags)
                        if (Slot.GameplayTags.HasTag(Tag)) { Total++; break; }
                }
            }
        }
    }
    return Total;
}