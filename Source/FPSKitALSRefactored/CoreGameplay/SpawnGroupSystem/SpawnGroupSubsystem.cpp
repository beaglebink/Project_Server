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
    ActiveGroupStates.Empty();
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
        Outcome.OutcomeSpawnGroup == EOutcomeSpawnGroup::SpawnGroupRegister)
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

void USpawnGroupSubsystem::ActivateSpawnGroupInternal(const FSpawnGroupId& GroupId)
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByGroupId(GroupId);
    if (Spawner)
        Spawner->SpawnGroupInternal();
}

void USpawnGroupSubsystem::ClearSpawnGroupInternal(const FSpawnGroupId& GroupId, ESpawnGroupResolutionReason Reason)
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByGroupId(GroupId);
    if (Spawner)
        Spawner->ClearGroup(Reason);
}

void USpawnGroupSubsystem::ResetSpawnGroupInternal(const FSpawnGroupId& GroupId)
{
    ASpawnGroupSpawner* Spawner = FindSpawnerByGroupId(GroupId);
    if (Spawner)
        Spawner->ResetGroup();
}

ASpawnGroupSpawner* USpawnGroupSubsystem::FindSpawnerByItemId(const FGuid& ItemId) const
{
    const TWeakObjectPtr<ASpawnGroupSpawner>* Found = SpawnerByItemId.Find(ItemId);
    return (Found && Found->IsValid()) ? Found->Get() : nullptr;
}

ASpawnGroupSpawner* USpawnGroupSubsystem::FindSpawnerByGroupId(const FSpawnGroupId& GroupId) const
{
    const FGuid* ItemId = GroupIdToItemId.Find(GroupId);
    return ItemId ? FindSpawnerByItemId(*ItemId) : nullptr;
}

void USpawnGroupSubsystem::HandleLevelLoaded(const FOutcomeEventBase& Outcome)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (Outcome.OutcomeType == EOutcomeType::Interior &&
        Outcome.OutcomeInterior == EOutcomeInterior::LevelLoaded)
    {
        ULevelLoadedPayload* Payload = Cast<ULevelLoadedPayload>(Outcome.Payload);
        if (Payload)
        {
            TArray<AActor*> AllActors;
            UGameplayStatics::GetAllActorsOfClass(World, ASpawnGroupSpawner::StaticClass(), AllActors);
            for (AActor* Actor : AllActors)
            {
                if (!IsValid(Actor)) continue;
                if (UFloorAssignmentComponent* C = Actor->FindComponentByClass<UFloorAssignmentComponent>())
                {
                    ASpawnGroupSpawner* Spawner = FindSpawnerByItemId(C->ItemId);
                    if (Spawner->CurrentStatus == ESpawnGroupStatus::Suppressed)
                    {
                        continue;
                    }

                    if (Spawner->CurrentStatus != ESpawnGroupStatus::Cleared && Spawner->CurrentStatus != ESpawnGroupStatus::Inactive)
                    {
                        Spawner->SpawnGroup();
                    }
                }
            }
        }
    }
}

void USpawnGroupSubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
    OutData.SubsystemName = GetSaveSubsystemName();
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

    TSharedPtr<FJsonObject> GroupsObj = MakeShared<FJsonObject>();
    for (const auto& FloorPair : PersistentGroupStates)
    {
        FString KeyStr = FString::Printf(TEXT("%s|%s"), *FloorPair.Key.InteriorSetId.ToString(), *FloorPair.Key.FloorId.ToString());
        TSharedPtr<FJsonObject> FloorGroups = MakeShared<FJsonObject>();
        for (const auto& GroupPair : FloorPair.Value)
        {
            TSharedPtr<FJsonObject> StateObj = MakeShared<FJsonObject>();
            StateObj->SetStringField(TEXT("GroupId"), GroupPair.Key.Id.ToString());
            StateObj->SetNumberField(TEXT("Status"), static_cast<uint8>(GroupPair.Value.Status));
            StateObj->SetNumberField(TEXT("ResolutionReason"), static_cast<uint8>(GroupPair.Value.ResolutionReason));
            StateObj->SetStringField(TEXT("LastMissionContext"), GroupPair.Value.LastMissionContext.ToString());
            StateObj->SetNumberField(TEXT("VisitIndex"), GroupPair.Value.VisitIndex);
            FloorGroups->SetObjectField(GroupPair.Key.Id.ToString(), StateObj);
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

            TMap<FSpawnGroupId, FSpawnGroupState> FloorGroups;
            const TSharedPtr<FJsonObject>* FloorGroupsObj = nullptr;
            if (FloorPair.Value->TryGetObject(FloorGroupsObj))
            {
                for (const auto& GroupPair : (*FloorGroupsObj)->Values)
                {
                    FGuid GroupGuid;
                    if (!FGuid::Parse(GroupPair.Key, GroupGuid)) continue;
                    FSpawnGroupId GroupId(GroupGuid);
                    const TSharedPtr<FJsonObject>* StateObj = nullptr;
                    if (GroupPair.Value->TryGetObject(StateObj))
                    {
                        FSpawnGroupState State;
                        if ((*StateObj)->HasField(TEXT("Status")))
                            State.Status = static_cast<ESpawnGroupStatus>((*StateObj)->GetIntegerField(TEXT("Status")));
                        if ((*StateObj)->HasField(TEXT("ResolutionReason")))
                            State.ResolutionReason = static_cast<ESpawnGroupResolutionReason>((*StateObj)->GetIntegerField(TEXT("ResolutionReason")));
                        if ((*StateObj)->HasField(TEXT("LastMissionContext")))
                            State.LastMissionContext = FName(*(*StateObj)->GetStringField(TEXT("LastMissionContext")));
                        if ((*StateObj)->HasField(TEXT("VisitIndex")))
                            State.VisitIndex = (*StateObj)->GetIntegerField(TEXT("VisitIndex"));
                        FloorGroups.Add(GroupId, State);
                    }
                }
            }
            PersistentGroupStates.Add(Key, FloorGroups);
        }
    }

    bIsLoadComplete = true;
}