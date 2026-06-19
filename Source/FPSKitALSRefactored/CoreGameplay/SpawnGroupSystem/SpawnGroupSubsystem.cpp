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
}

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

    // Загружаем ассет этажа, с которого уходим
    UFloorAsset* SourceFloorAsset = Payload->SourceFloor.LoadSynchronous();
    if (!SourceFloorAsset) return;

    FGuid FloorId = SourceFloorAsset->FloorID;
    FGuid InteriorSetId = SourceFloorAsset->InteriorSetID;
    if (!InteriorSetId.IsValid())
    {
        // Если InteriorSetID не задан, пробуем взять из родительского ассета
        if (UInteriorSetAsset* Parent = SourceFloorAsset->ParentInteriorSet.LoadSynchronous())
            InteriorSetId = Parent->InteriorSetID;
    }
    if (!FloorId.IsValid() || !InteriorSetId.IsValid()) return;

    FInteriorFloorKey LeavingKey(InteriorSetId, FloorId);

    // Обновляем состояние всех спавнеров, принадлежащих этому этажу
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


void USpawnGroupSubsystem::ActivateSpawnGroupInternal(const FGuid& GroupId)
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByGroupId(GroupId);
    if (Spawner)
        Spawner->SpawnGroupInternal();
}

void USpawnGroupSubsystem::ClearSpawnGroupInternal(const FGuid& GroupId, ESpawnGroupResolutionReason Reason)
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByGroupId(GroupId);
    if (Spawner)
        Spawner->ClearGroup(Reason);
}

void USpawnGroupSubsystem::ResetSpawnGroupInternal(const FGuid& GroupId)
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByGroupId(GroupId);
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

    if (State.bStoreSpawnParameters)
        State.Slots = Spawner->CaptureCurrentSlots();
    else
        State.TypeKilled = Spawner->GetTypeKilled();



    TMap<FGuid, FSpawnGroupState>& FloorStates = PersistentGroupStates.FindOrAdd(FloorKey);
    FloorStates.Add(Spawner->GetRuntimeGroupId(), State);
}

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

void USpawnGroupSubsystem::HandleLevelLoaded(const FOutcomeEventBase& Outcome)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (Outcome.OutcomeType != EOutcomeType::Interior ||
        Outcome.OutcomeInterior != EOutcomeInterior::LevelLoaded)
        return;

    for (const auto& Pair : SpawnerByItemId)
    {
        ASpawnGroupSpawner* Spawner = Pair.Value.Get();
        if (!Spawner || !IsValid(Spawner)) continue;

        FInteriorFloorKey FloorKey = GetFloorKeyFromSpawner(Spawner);
        if (!FloorKey.FloorId.IsValid()) continue;

        const TMap<FGuid, FSpawnGroupState>* FloorStates = PersistentGroupStates.Find(FloorKey);
        if (!FloorStates) continue;


        if (Spawner->FloorAssignmentComp)
        {
            const FSpawnGroupState* State = FloorStates->Find(Spawner->GetRuntimeGroupId());
            if (!State) continue;

            // Восстанавливаем флаг спавнера (если изменился в рантайме)
            Spawner->IsStoreSpawnParameters = State->bStoreSpawnParameters;

            if (State->bStoreSpawnParameters)
            {
				Spawner->BlockNewSpawn = true;
                Spawner->RestoreFromSlots(State->Slots);
                return;
            }
            else
            {
                Spawner->BlockNewSpawn = true;
                Spawner->RestoreFromState(*State);
            }

            ESpawnGroupStatus Status = Spawner->GetCurrentStatus();
            if (Spawner->IsFullRespawn || (Status == ESpawnGroupStatus::Active || Status == ESpawnGroupStatus::PartiallyCleared))
            {
                Spawner->BlockNewSpawn = true;
                Spawner->SpawnGroup();
            }
        }
    }
}

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
        State.KilledCount = Spawner->GetSpawnedCount();

        if (Spawner->IsStoreSpawnParameters)
        {
            State.Slots = Spawner->CaptureCurrentSlots();
        }
        else
        {
            State.TypeKilled = Spawner->GetTypeKilled();
        }

        TMap<FGuid, FSpawnGroupState>& FloorStates = PersistentGroupStates.FindOrAdd(FloorKey);
        FloorStates.Add(Spawner->GetRuntimeGroupId(), State);
    }

    // --- Сериализация в JSON (как ранее, с добавлением поля bStoreSpawnParameters и Slots) ---
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

            if (GroupPair.Value.bStoreSpawnParameters)
            {
                // Сериализация Slots
                TArray<TSharedPtr<FJsonValue>> SlotsArray;
                for (const FSpawnSlotState& Slot : GroupPair.Value.Slots)
                {
                    TSharedPtr<FJsonObject> SlotObj = MakeShared<FJsonObject>();
                    SlotObj->SetStringField(TEXT("ItemId"), Slot.ItemId.ToString());
                    SlotObj->SetStringField(TEXT("ActorClass"), Slot.ActorClass ? Slot.ActorClass->GetPathName() : TEXT(""));
                    // Сериализация трансформа
                    FVector Loc = Slot.SpawnTransform.GetLocation();
                    FRotator Rot = Slot.SpawnTransform.Rotator();
                    SlotObj->SetArrayField(TEXT("Location"), { MakeShared<FJsonValueNumber>(Loc.X), MakeShared<FJsonValueNumber>(Loc.Y), MakeShared<FJsonValueNumber>(Loc.Z) });
                    SlotObj->SetArrayField(TEXT("Rotation"), { MakeShared<FJsonValueNumber>(Rot.Pitch), MakeShared<FJsonValueNumber>(Rot.Yaw), MakeShared<FJsonValueNumber>(Rot.Roll) });
                    SlotObj->SetBoolField(TEXT("bIsAlive"), Slot.bIsAlive);
                    SlotsArray.Add(MakeShared<FJsonValueObject>(SlotObj));
                }
                StateObj->SetArrayField(TEXT("Slots"), SlotsArray);
            }
            else
            {
                // Сериализация TypeKilled
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
            // Ключ: "InteriorSetId|FloorId"
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
                    // Ключ: GroupId (GUID строкой)
                    FGuid GroupGuid;
                    if (!FGuid::Parse(GroupPair.Key, GroupGuid)) continue;
                    FGuid GroupId(GroupGuid);

                    const TSharedPtr<FJsonObject>* StateObj = nullptr;
                    if (!GroupPair.Value->TryGetObject(StateObj)) continue;

                    FSpawnGroupState State;
                    State.GroupId = GroupId;

                    // Статус
                    if ((*StateObj)->HasField(TEXT("Status")))
                        State.Status = static_cast<ESpawnGroupStatus>((*StateObj)->GetIntegerField(TEXT("Status")));
                    // ResolutionReason
                    if ((*StateObj)->HasField(TEXT("ResolutionReason")))
                        State.ResolutionReason = static_cast<ESpawnGroupResolutionReason>((*StateObj)->GetIntegerField(TEXT("ResolutionReason")));
                    // LastMissionContext
                    if ((*StateObj)->HasField(TEXT("LastMissionContext")))
                        State.LastMissionContext = FName(*(*StateObj)->GetStringField(TEXT("LastMissionContext")));
                    // VisitIndex
                    if ((*StateObj)->HasField(TEXT("VisitIndex")))
                        State.VisitIndex = (*StateObj)->GetIntegerField(TEXT("VisitIndex"));
                    if ((*StateObj)->HasField(TEXT("KilledCount")))
                        State.KilledCount = (*StateObj)->GetIntegerField(TEXT("KilledCount"));
                    // bStoreSpawnParameters
                    if ((*StateObj)->HasField(TEXT("bStoreSpawnParameters")))
                        State.bStoreSpawnParameters = (*StateObj)->GetBoolField(TEXT("bStoreSpawnParameters"));
                    else
                        State.bStoreSpawnParameters = false; // совместимость со старыми сохранениями

                    if (State.bStoreSpawnParameters)
                    {
                        // Десериализация Slots
                        const TArray<TSharedPtr<FJsonValue>>* SlotsArray = nullptr;
                        if ((*StateObj)->TryGetArrayField(TEXT("Slots"), SlotsArray))
                        {
                            for (const TSharedPtr<FJsonValue>& SlotVal : *SlotsArray)
                            {
                                const TSharedPtr<FJsonObject>* SlotObj = nullptr;
                                if (!SlotVal->TryGetObject(SlotObj)) continue;
                                FSpawnSlotState Slot;
                                // ItemId
                                FString ItemIdStr;
                                if ((*SlotObj)->TryGetStringField(TEXT("ItemId"), ItemIdStr))
                                    FGuid::Parse(ItemIdStr, Slot.ItemId);
                                // ActorClass
                                FString ClassPath;
                                if ((*SlotObj)->TryGetStringField(TEXT("ActorClass"), ClassPath) && !ClassPath.IsEmpty())
                                    Slot.ActorClass = LoadClass<AActor>(nullptr, *ClassPath);
                                // SpawnTransform
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
                                // bIsAlive
                                (*SlotObj)->TryGetBoolField(TEXT("bIsAlive"), Slot.bIsAlive);
                                // Доп. поля можно добавить позже (например, ActorPropertiesJSON)
                                State.Slots.Add(Slot);
                            }
                        }
                    }
                    else
                    {
                        // Десериализация TypeKilled
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
                    }

                    FloorGroups.Add(GroupId, State);
                }
            }
            PersistentGroupStates.Add(Key, FloorGroups);
        }
    }

    bIsLoadComplete = true;
}

FInteriorFloorKey USpawnGroupSubsystem::GetFloorKeyFromSpawner(ASpawnGroupSpawner* Spawner) const
{
    if (!Spawner) return FInteriorFloorKey();
    UFloorAssignmentComponent* Comp = Spawner->FindComponentByClass<UFloorAssignmentComponent>();
    if (!Comp) return FInteriorFloorKey();
    return FInteriorFloorKey(Comp->InteriorSetId, Comp->FloorId);
}

