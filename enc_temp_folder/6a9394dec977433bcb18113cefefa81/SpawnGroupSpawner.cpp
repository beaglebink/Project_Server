#include "SpawnGroupSpawner.h"
#include "SpawnVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/ChildActorComponent.h"
#include "GameFramework/PlayerController.h"
#include "../InteriorInstanceSystem/FloorAssignmentComponent.h"
#include "../InteriorInstanceSystem/InteriorSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../SpawnGroupSystem/GhostClearedPayload.h"
#include "SpawnGroupRegistrationPayload.h"
#include "GhostCapturedPayload.h"
// ============================================================================
// Вспомогательные функции для сериализации (скопированы из InteriorSubsystem)
// ============================================================================
namespace SpawnGroupSerialization
{
    static TSharedPtr<FJsonObject> TransformToJsonObject(const FTransform& Transform)
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        FVector Loc = Transform.GetLocation();
        FRotator Rot = Transform.Rotator();
        FVector Scale = Transform.GetScale3D();
        Obj->SetArrayField(TEXT("Location"), {
            MakeShared<FJsonValueNumber>(Loc.X),
            MakeShared<FJsonValueNumber>(Loc.Y),
            MakeShared<FJsonValueNumber>(Loc.Z)
            });
        Obj->SetArrayField(TEXT("Rotation"), {
            MakeShared<FJsonValueNumber>(Rot.Pitch),
            MakeShared<FJsonValueNumber>(Rot.Yaw),
            MakeShared<FJsonValueNumber>(Rot.Roll)
            });
        Obj->SetArrayField(TEXT("Scale"), {
            MakeShared<FJsonValueNumber>(Scale.X),
            MakeShared<FJsonValueNumber>(Scale.Y),
            MakeShared<FJsonValueNumber>(Scale.Z)
            });
        return Obj;
    }

    static TSharedPtr<FJsonObject> PropertyEntryToJson(const FFloorSavedPropertyEntry& Entry)
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("PropertyName"), Entry.PropertyName.ToString());
        Obj->SetStringField(TEXT("ValueText"), Entry.ValueText);
        return Obj;
    }

    static TSharedPtr<FJsonObject> ComponentStateToJson(const FFloorSavedComponentState& State)
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("ComponentName"), State.ComponentName.ToString());
        Obj->SetStringField(TEXT("ComponentClassName"), State.ComponentClassName.ToString());
        Obj->SetBoolField(TEXT("bWasActive"), State.bWasActive);
        Obj->SetBoolField(TEXT("bWasAttached"), State.bWasAttached);
        Obj->SetStringField(TEXT("AttachParentName"), State.AttachParentName.ToString());
        Obj->SetStringField(TEXT("AttachSocketName"), State.AttachSocketName.ToString());
        Obj->SetObjectField(TEXT("RelativeTransform"), TransformToJsonObject(State.RelativeTransform));
        Obj->SetObjectField(TEXT("WorldTransform"), TransformToJsonObject(State.WorldTransform));
        Obj->SetBoolField(TEXT("bHasRelativeTransform"), State.bHasRelativeTransform);
        Obj->SetBoolField(TEXT("bHasWorldTransform"), State.bHasWorldTransform);
        Obj->SetBoolField(TEXT("bWasSimulatingPhysics"), State.bWasSimulatingPhysics);
        Obj->SetArrayField(TEXT("SavedLinearVelocity"), {
            MakeShared<FJsonValueNumber>(State.SavedLinearVelocity.X),
            MakeShared<FJsonValueNumber>(State.SavedLinearVelocity.Y),
            MakeShared<FJsonValueNumber>(State.SavedLinearVelocity.Z)
            });
        Obj->SetArrayField(TEXT("SavedAngularVelocityDeg"), {
            MakeShared<FJsonValueNumber>(State.SavedAngularVelocityDeg.X),
            MakeShared<FJsonValueNumber>(State.SavedAngularVelocityDeg.Y),
            MakeShared<FJsonValueNumber>(State.SavedAngularVelocityDeg.Z)
            });

        TArray<TSharedPtr<FJsonValue>> PropsArray;
        for (const auto& Prop : State.Properties)
            PropsArray.Add(MakeShared<FJsonValueObject>(PropertyEntryToJson(Prop)));
        Obj->SetArrayField(TEXT("Properties"), PropsArray);
        return Obj;
    }

    static void CollectSaveGameProperties(UObject* Obj, TArray<FFloorSavedPropertyEntry>& OutProps)
    {
        if (!IsValid(Obj)) return;
        for (TFieldIterator<FProperty> It(Obj->GetClass()); It; ++It)
        {
            FProperty* Prop = *It;
            if (!Prop->HasAllPropertyFlags(CPF_SaveGame)) continue;

            FString Exported;
            Prop->ExportText_InContainer(0, Exported, Obj, nullptr, Obj, PPF_None);
            OutProps.Add({ Prop->GetFName(), Exported });
        }
    }
}

namespace SpawnGroupSerialization
{
    // --- Дополнительные функции для десериализации ---

    static FTransform TransformFromJsonObject(const TSharedPtr<FJsonObject>& Obj)
    {
        if (!Obj.IsValid()) return FTransform::Identity;
        auto ReadVector = [](const TSharedPtr<FJsonObject>& O, const FString& Field) -> FVector
            {
                if (!O->HasField(Field)) return FVector::ZeroVector;
                const TArray<TSharedPtr<FJsonValue>>& Arr = O->GetArrayField(Field);
                if (Arr.Num() >= 3)
                    return FVector(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
                return FVector::ZeroVector;
            };
        FVector Loc = ReadVector(Obj, TEXT("Location"));
        FRotator Rot;
        if (Obj->HasField(TEXT("Rotation")))
        {
            const TArray<TSharedPtr<FJsonValue>>& Arr = Obj->GetArrayField(TEXT("Rotation"));
            if (Arr.Num() >= 3)
                Rot = FRotator(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
        }
        FVector Scale = ReadVector(Obj, TEXT("Scale"));
        return FTransform(Rot, Loc, Scale);
    }

    static FFloorSavedPropertyEntry PropertyEntryFromJson(const TSharedPtr<FJsonObject>& Obj)
    {
        FFloorSavedPropertyEntry Entry;
        if (Obj.IsValid())
        {
            Entry.PropertyName = FName(*Obj->GetStringField(TEXT("PropertyName")));
            Entry.ValueText = Obj->GetStringField(TEXT("ValueText"));
        }
        return Entry;
    }

    static FFloorSavedComponentState ComponentStateFromJson(const TSharedPtr<FJsonObject>& Obj)
    {
        FFloorSavedComponentState State;
        if (!Obj.IsValid()) return State;
        State.ComponentName = FName(*Obj->GetStringField(TEXT("ComponentName")));
        State.ComponentClassName = FName(*Obj->GetStringField(TEXT("ComponentClassName")));
        State.bWasActive = Obj->GetBoolField(TEXT("bWasActive"));
        State.bWasAttached = Obj->GetBoolField(TEXT("bWasAttached"));
        State.AttachParentName = FName(*Obj->GetStringField(TEXT("AttachParentName")));
        State.AttachSocketName = FName(*Obj->GetStringField(TEXT("AttachSocketName")));
        if (Obj->HasField(TEXT("RelativeTransform")))
            State.RelativeTransform = TransformFromJsonObject(Obj->GetObjectField(TEXT("RelativeTransform")));
        if (Obj->HasField(TEXT("WorldTransform")))
            State.WorldTransform = TransformFromJsonObject(Obj->GetObjectField(TEXT("WorldTransform")));
        State.bHasRelativeTransform = Obj->GetBoolField(TEXT("bHasRelativeTransform"));
        State.bHasWorldTransform = Obj->GetBoolField(TEXT("bHasWorldTransform"));
        State.bWasSimulatingPhysics = Obj->GetBoolField(TEXT("bWasSimulatingPhysics"));

        if (Obj->HasField(TEXT("SavedLinearVelocity")))
        {
            const TArray<TSharedPtr<FJsonValue>>& Arr = Obj->GetArrayField(TEXT("SavedLinearVelocity"));
            if (Arr.Num() >= 3)
                State.SavedLinearVelocity = FVector(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
        }
        if (Obj->HasField(TEXT("SavedAngularVelocityDeg")))
        {
            const TArray<TSharedPtr<FJsonValue>>& Arr = Obj->GetArrayField(TEXT("SavedAngularVelocityDeg"));
            if (Arr.Num() >= 3)
                State.SavedAngularVelocityDeg = FVector(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
        }

        if (Obj->HasField(TEXT("Properties")))
        {
            const TArray<TSharedPtr<FJsonValue>>& PropsArray = Obj->GetArrayField(TEXT("Properties"));
            for (const auto& Val : PropsArray)
                if (Val->Type == EJson::Object)
                    State.Properties.Add(PropertyEntryFromJson(Val->AsObject()));
        }
        return State;
    }

    static void ApplySaveGameProperties(UObject* Obj, const TArray<FFloorSavedPropertyEntry>& Props)
    {
        if (!IsValid(Obj)) return;
        for (const FFloorSavedPropertyEntry& Entry : Props)
        {
            FProperty* Prop = FindFProperty<FProperty>(Obj->GetClass(), Entry.PropertyName);
            if (!Prop) continue;
            Prop->ImportText_InContainer(*Entry.ValueText, Obj, Obj, PPF_None);
        }
    }

    static void RestoreSceneComponentTransform(USceneComponent* Comp, const FFloorSavedComponentState& CState)
    {
        if (!IsValid(Comp)) return;

        if (CState.bWasAttached)
        {
            // Привязка уже восстановлена на первом проходе, но на всякий случай
            // Можно пропустить, так как мы уже сделали аттачмент.
        }

        if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Comp))
        {
            const bool bWasSim = CState.bWasSimulatingPhysics;
            if (bWasSim)
            {
                Prim->SetSimulatePhysics(false);

                if (CState.bHasRelativeTransform)
                    Comp->SetRelativeTransform(CState.RelativeTransform, false, nullptr, ETeleportType::TeleportPhysics);
                else if (CState.bHasWorldTransform && !CState.bWasAttached)
                    Comp->SetWorldTransform(CState.WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);

                Prim->SetSimulatePhysics(true);
                Prim->SetPhysicsLinearVelocity(CState.SavedLinearVelocity);
                Prim->SetPhysicsAngularVelocityInDegrees(CState.SavedAngularVelocityDeg);
            }
            else
            {
                if (CState.bHasRelativeTransform)
                    Comp->SetRelativeTransform(CState.RelativeTransform, false, nullptr, ETeleportType::None);
                else if (CState.bHasWorldTransform && !CState.bWasAttached)
                    Comp->SetWorldTransform(CState.WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
            }
        }
        else
        {
            if (CState.bHasRelativeTransform)
                Comp->SetRelativeTransform(CState.RelativeTransform, false, nullptr, ETeleportType::None);
            else if (CState.bHasWorldTransform && !CState.bWasAttached)
                Comp->SetWorldTransform(CState.WorldTransform, false, nullptr, ETeleportType::TeleportPhysics);
        }
    }
}

ASpawnGroupSpawner::ASpawnGroupSpawner()
{
    PrimaryActorTick.bCanEverTick = true;
#if WITH_EDITOR
    PrimaryActorTick.bStartWithTickEnabled = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
#endif
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    FloorAssignmentComp = CreateDefaultSubobject<UFloorAssignmentComponent>(TEXT("FloorAssignment"));
    if (FloorAssignmentComp)
    {
        FloorAssignmentComp->SnapshotChannel = ESnapshotChannel::Snapshot;
        FloorAssignmentComp->ActorType = EFloorActorType::SpawnGroupSpawner;
    }
}

void ASpawnGroupSpawner::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

    if (FloorAssignmentComp && !FloorAssignmentComp->ItemId.IsValid())
    {
        FloorAssignmentComp->ItemId = FGuid::NewGuid();
        FloorAssignmentComp->ProtectedItemId = FloorAssignmentComp->ItemId;
        FloorAssignmentComp->SnapshotChannel = ESnapshotChannel::Snapshot;
        // Generated new ItemId for duplicate
        // Generated new ItemId for duplicate
        UE_LOG(LogTemp, Verbose, TEXT("SpawnGroupSpawner [%s]: Generated new ItemId for duplicate"), *GetName());
    }
}

void ASpawnGroupSpawner::BeginPlay()
{
    Super::BeginPlay();

    UFloorAssignmentComponent* FloorComp = FindComponentByClass<UFloorAssignmentComponent>();
    if (!FloorComp)
    {
        FloorComp = NewObject<UFloorAssignmentComponent>(this);
        FloorComp->RegisterComponent();
    }

    if (!FloorComp->ItemId.IsValid())
    {
        FloorComp->ItemId = FGuid::NewGuid();
        FloorComp->ProtectedItemId = FloorComp->ItemId;
    }
    FloorComp->SnapshotChannel = ESnapshotChannel::Snapshot;
    FloorComp->ActorType = EFloorActorType::SpawnGroupSpawner;

    if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        if (SpawnGroupAsset)
        {
            USpawnGroupRegistrationPayload* Payload = EventBus->CreatePayload<USpawnGroupRegistrationPayload>();
            UFloorAssignmentComponent* Comp = FindComponentByClass<UFloorAssignmentComponent>();
            if (Comp)
            {
                Payload->Setup(Comp->ItemId);
            }

            FOutcomeEventBase Ev;
            Ev.OutcomeType = EOutcomeType::SpawnGroup;
            Ev.OutcomeSpawnGroup = EOutcomeSpawnGroup::SpawnGroupRegister;
            Ev.Payload = Payload;
            EventBus->PublishOutcome(Ev);
        }
    }
}

void ASpawnGroupSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UFloorAssignmentComponent* FloorComp = FindComponentByClass<UFloorAssignmentComponent>())
    {
        if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
        {
            if (SpawnGroupAsset)
            {
                USpawnGroupRegistrationPayload* Payload = EventBus->CreatePayload<USpawnGroupRegistrationPayload>();
                Payload->Setup(FloorComp->ItemId);

                FOutcomeEventBase Ev;
                Ev.OutcomeType = EOutcomeType::SpawnGroup;
                Ev.OutcomeSpawnGroup = EOutcomeSpawnGroup::SpawnGroupUnregister;
                Ev.Payload = Payload;
                EventBus->PublishOutcome(Ev);
            }
        }
    }

    Super::EndPlay(EndPlayReason);
}

void ASpawnGroupSpawner::SetStates(const FSpawnGroupState& State)
{
    StoredState = State;
}

void ASpawnGroupSpawner::StoreSpawnParameters()
{
    for (AActor* Ghost : SpawnedGhosts)
    {
        if (!IsValid(Ghost)) continue;
        StoredSpawnParameters.Add(Ghost->GetClass(), Ghost->GetActorTransform());
    }
}

void ASpawnGroupSpawner::SafeDestroyAllGhosts()
{
    for (int32 i = SpawnedGhosts.Num() - 1; i >= 0; --i)
    {
        AAlsCharacter* Ghost = SpawnedGhosts[i].Get();
        if (IsValid(Ghost))
        {
            Ghost->OnDestroyed.RemoveDynamic(this, &ASpawnGroupSpawner::OnGhostDestroyed);
            Ghost->Destroy();
        }
    }
    SpawnedGhosts.Empty();
    SpawnedCount = 0;

    if (GetWorld())
        GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
}

int32 ASpawnGroupSpawner::GetNeedSpasnedCount()
{
    int32 NeedSpawnCount = 0;
    if (SpawnGroupAsset->Composition.bUsePool)
    {
        for (auto Pool : SpawnGroupAsset->Composition.ActorsPool)
        {
            NeedSpawnCount += Pool.Count;
        }
    }
    else
    {
        NeedSpawnCount = SpawnGroupAsset->Composition.Count;
    }
    return NeedSpawnCount;
}

ASpawnVolume* ASpawnGroupSpawner::GetRandomSpawnLocation() const
{
    if (SpawnLocations.Num() == 0) return nullptr;
    const int32 Index = FMath::RandRange(0, SpawnLocations.Num() - 1);
    return SpawnLocations[Index];
}

FTransform ASpawnGroupSpawner::GetTransformFromLocation(ASpawnVolume* Volume) const
{
    FVector SpawnLocation;
    FRotator SpawnRotation = DefaultSpawnRotation;

    if (Volume)
    {
        SpawnLocation = Volume->GetRandomSpawnPoint();
        SpawnRotation = Volume->GetRandomRotation();
    }
    else
    {
        SpawnLocation = GetActorLocation();
    }

    SpawnLocation += SpawnOffset;
    return FTransform(SpawnRotation, SpawnLocation, FVector::OneVector);
}

void ASpawnGroupSpawner::SpawnGroupInternal()
{
    if (!Restore)
        SpawnGroup();
}

TArray<FSpawnSlotState> ASpawnGroupSpawner::CaptureCurrentSlots() const
{
    TArray<FSpawnSlotState> Slots;
    if (!IsStoreSpawnParameters) return Slots;

    for (AActor* Ghost : SpawnedGhosts)
    {
        if (!IsValid(Ghost)) continue;

        FSpawnSlotState Slot;
        if (UFloorAssignmentComponent* Comp = Ghost->FindComponentByClass<UFloorAssignmentComponent>())
        {
            Slot.ItemId = Comp->ItemId;
        }
        else
        {
            Slot.ItemId = FGuid::NewGuid();
        }

        Slot.ActorClass = Ghost->GetClass();
        Slot.SpawnTransform = Ghost->GetActorTransform();
        // fixed: instead of bIsAlive
        // исправлено: вместо bIsAlive
        Slot.State = EGhostState::Alive;

        Slots.Add(Slot);
    }
    return Slots;
}

void ASpawnGroupSpawner::RestoreFromState(const FSpawnGroupState& State)
{
    SafeDestroyAllGhosts();

    if (CurrentStatus == ESpawnGroupStatus::Suppressed || CurrentStatus == ESpawnGroupStatus::Inactive)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Group CurrentStatus Suppressed, skip spawn"), *GetName());
        return;
    }

    if (CurrentStatus == ESpawnGroupStatus::Cleared)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Group already Cleared, skip spawn"), *GetName());
        return;
    }

    if (!IsFullRespawn)
    {
        SafeDestroyAllGhosts();
        TypeKilled = State.TypeKilled;
        KilledCount = 0;
        for (const auto& Pair : TypeKilled) KilledCount += Pair.Value;
    }

    CurrentStatus = State.Status;
    if (CurrentStatus == ESpawnGroupStatus::Cleared)
    {
        ClearGroup(State.ResolutionReason);
        SpawnedCount = 0;
    }

    UE_LOG(LogTemp, Log, TEXT("SpawnGroupSpawner [%s]: Restored from state (TypeKilled: %d)"), *GetName(), TypeKilled.Num());
}

void ASpawnGroupSpawner::SpawnGroup()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: World is null"), *GetName());
        return;
    }

    if (!SpawnGroupAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: SpawnGroupAsset is null"), *GetName());
        return;
    }
    if (CurrentStatus == ESpawnGroupStatus::Suppressed)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Group CurrentStatus Suppressed, skip spawn"), *GetName());
        return;
    }

    if (Restore && CurrentStatus == ESpawnGroupStatus::Cleared)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Group already Cleared, skip spawn"), *GetName());
        return;
    }

    if (SpawnGroupAsset->Composition.bUsePool)
    {
        // Desired count of each class
        // Желаемое количество каждого класса
        TMap<UClass*, int32> DesiredCounts;

        for (const FSpawnTypeCount& TypeCount : SpawnGroupAsset->Composition.ActorsPool)
        {
            DesiredCounts.Add(TypeCount.ActorClass, TypeCount.Count);
        }

        TArray<TSubclassOf<AActor>> ClassesToSpawn;

        // Subtract already killed (TypeKilled)
        // Вычитаем уже убитых (TypeKilled)

        for (const auto& Pair : DesiredCounts)
        {
            UClass* Class = Pair.Key;
            int32 Desired = Pair.Value;
            int32 Killed = TypeKilled.FindRef(Class->GetFName());
            int32 Need = FMath::Max(0, Desired - Killed);
            for (int32 i = 0; i < Need; ++i)
            {
                ClassesToSpawn.Add(Class);
            }
        }

        SpawnedCount = SpawnedGhosts.Num();
        int32 NeedSpawnCount = 0;

        if (SpawnGroupAsset->Composition.bUsePool)
        {
            for (auto Pool : SpawnGroupAsset->Composition.ActorsPool)
            {
                NeedSpawnCount = NeedSpawnCount + Pool.Count;
            }
        }
        else
        {
            NeedSpawnCount = SpawnGroupAsset->Composition.Count;
        }

        FTimerDelegate Delegate = FTimerDelegate::CreateLambda([this, ClassesToSpawn, DesiredCounts, NeedSpawnCount, World]()
            {
                ASpawnVolume* Location = GetRandomSpawnLocation();

                if (ClassesToSpawn.Num() <= 0 || !ClassesToSpawn.IsValidIndex(SpawnedCount))
                {
                    UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Invalid index for FinalClasses"), *GetName());
                    World->GetTimerManager().ClearTimer(TimerHandle);
                    AllSpawned = true;
                    return;
                }

                if (NeedSpawnCount <= SpawnedCount + KilledCount + GetCapturedCount())
                {
                    //KilledCount = SpawnGroupAsset->Composition.Count;//DesiredCounts.Num();
                    World->GetTimerManager().ClearTimer(TimerHandle);
                    UE_LOG(LogTemp, Log, TEXT("SpawnGroupSpawner [%s]: Spawned %d ghosts"),
                        *GetName(), SpawnedGhosts.Num());
                    AllSpawned = true;
                    return;
                }

                TSubclassOf<AActor> SpawnedClass = ClassesToSpawn[SpawnedCount];
                FTransform SpawnTransform;

                if (IsStoreSpawnParameters && StoredSpawnParameters.Num() > 0)
                {
                    SpawnTransform = StoredSpawnParameters.FindRef(SpawnedClass);
                    StoredSpawnParameters.Remove(SpawnedClass);
                }
                else
                {
                    SpawnTransform = GetTransformFromLocation(Location);
                }

                AActor* Ghost = SpawnSingleGhost(SpawnedClass, SpawnTransform);
                if (Ghost)
                {
                    UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Spawned Ghost [%s] Type [%s]"), *GetName(), *Ghost->GetName(), *SpawnedClass->GetName());
                    SpawnedCount++;

                    if (const FSpawnedEnemyState* State = FindStateForActor(Ghost))
                    {
                        DeserializeAndApplyState(Ghost, *State);
                        UE_LOG(LogTemp, Log, TEXT("SpawnGroupSpawner [%s]: Applied saved state for ItemId %s"),
                            *GetName(), *State->ItemId.ToString());
                    }

                    SpawnedGhosts.Add(Cast<AAlsCharacter>(Ghost));
                    Ghost->OnDestroyed.AddDynamic(this, &ASpawnGroupSpawner::OnGhostDestroyed);

                    OnGhostSpawned.Broadcast(Ghost);
                }

                UpdateGroupStatus();

            });

        World->GetTimerManager().SetTimer(TimerHandle, Delegate, SpawnInterval, true);
    }
    else
    {
        // Determine classes for spawning
        // Определяем классы для спавна
        TArray<TSubclassOf<AAlsCharacter>> ClassesToSpawn;
        int32 DesiredCount = 0;

        ClassesToSpawn = SpawnGroupAsset->Composition.ActorClasses;
        DesiredCount = SpawnGroupAsset->Composition.Count;


        if (ClassesToSpawn.Num() <= 0 || !ClassesToSpawn.IsValidIndex(SpawnedCount))
        {
            UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Invalid index for FinalClasses"), *GetName());
            World->GetTimerManager().ClearTimer(TimerHandle);
            return;
        }

        // If more Count is specified than classes, cycle through classes repeatedly
        // Если указано больше Count, чем классов, циклически повторяем классы
        TArray<TSubclassOf<AActor>> FinalClasses;
        for (int32 i = 0; i < DesiredCount; ++i)
        {
            FinalClasses.Add(ClassesToSpawn[i % ClassesToSpawn.Num()]);
        }

        // Spawn them
        // Спавним
        SpawnedGhosts.Empty();
        bHasPublishedClear = false;

        FTimerDelegate Delegate = FTimerDelegate::CreateLambda([this, FinalClasses, World, DesiredCount]()
            {
                ASpawnVolume* Location = GetRandomSpawnLocation();

                if (!FinalClasses.IsValidIndex(SpawnedCount % FinalClasses.Num()))
                {
                    UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Invalid index for FinalClasses"), *GetName());
                    World->GetTimerManager().ClearTimer(TimerHandle);
                    return;
                }

                if (DesiredCount <= KilledCount)
                {
                    KilledCount = DesiredCount;
                    World->GetTimerManager().ClearTimer(TimerHandle);
                    return;
                }

                if (SpawnedCount >= DesiredCount - KilledCount)
                {
                    // All ghosts spawned
                    // Все призраки заспавнены
                    World->GetTimerManager().ClearTimer(TimerHandle);
                    OnAllSpawned.Broadcast();

                    UE_LOG(LogTemp, Log, TEXT("SpawnGroupSpawner [%s]: Spawned %d ghosts"),
                        *GetName(), SpawnedGhosts.Num());
                    return;
                }

                TSubclassOf<AActor> SpawnedClass = FinalClasses[SpawnedCount % FinalClasses.Num()];
                FTransform SpawnTransform;

                if (IsStoreSpawnParameters && StoredSpawnParameters.Num() > 0)
                {
                    SpawnTransform = StoredSpawnParameters.FindRef(SpawnedClass);
                }
                else
                {
                    SpawnTransform = GetTransformFromLocation(Location);
                }

                if (FinalClasses.Num() == 0)
                {
                    UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: FinalClasses is empty, cannot spawn"), *GetName());
                    World->GetTimerManager().ClearTimer(TimerHandle);
                    return;
                }

                AActor* Ghost = SpawnSingleGhost(SpawnedClass, SpawnTransform);
                if (Ghost)
                {
                    UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Spawned Ghost [%s] Type [%s]"), *GetName(), *Ghost->GetName(), *SpawnedClass->GetName());
                    SpawnedCount++;

                    if (const FSpawnedEnemyState* State = FindStateForActor(Ghost))
                    {
                        DeserializeAndApplyState(Ghost, *State);
                        UE_LOG(LogTemp, Log, TEXT("SpawnGroupSpawner [%s]: Applied saved state for ItemId %s"),
                            *GetName(), *State->ItemId.ToString());
                    }

                    SpawnedGhosts.Add(Cast<AAlsCharacter>(Ghost));
                    Ghost->OnDestroyed.AddDynamic(this, &ASpawnGroupSpawner::OnGhostDestroyed);

                    OnGhostSpawned.Broadcast(Ghost);
                }

                UpdateGroupStatus();
            });

        World->GetTimerManager().SetTimer(TimerHandle, Delegate, SpawnInterval, true);
    }
}

void ASpawnGroupSpawner::ClearGroup(ESpawnGroupResolutionReason Reason)
{
    if (CurrentStatus == ESpawnGroupStatus::Cleared)
    {
        UE_LOG(LogTemp, Verbose, TEXT("SpawnGroupSpawner [%s]: Already cleared, skip"), *GetName());
        return;
    }

    // Mark all slots as killed (using State)
    // Помечаем все слоты как убитые (используем State)
    for (FSpawnSlotState& Slot : AllSlots)
        Slot.State = EGhostState::Killed;

    for (AActor* Ghost : SpawnedGhosts)
    {
        if (Ghost)
        {
            Ghost->OnDestroyed.RemoveDynamic(this, &ASpawnGroupSpawner::OnGhostDestroyed);
            Ghost->Destroy();
        }
    }
    SpawnedGhosts.Empty();

    PublishGhostClearedEvent(Reason);
    bHasPublishedClear = true;
    OnAllCleared.Broadcast(Reason);

    if (bDestroyOnClear)
        Destroy();

    CurrentStatus = ESpawnGroupStatus::Cleared;
}

void ASpawnGroupSpawner::ResetGroup()
{
    if (!bAllowRespawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Reset not allowed (bAllowRespawn=false)"), *GetName());
        return;
    }

    TypeKilled.Empty();
    KilledCount = 0;
    SpawnedCount = 0;
    AllSlots.Empty();

    ClearGroup(ESpawnGroupResolutionReason::Other);
    SpawnGroup();
}

int32 ASpawnGroupSpawner::GetAliveGhostCount() const
{
    int32 Alive = 0;
    for (AActor* Ghost : SpawnedGhosts)
    {
        if (IsValid(Ghost)) Alive++;
    }
    return Alive;
}

bool ASpawnGroupSpawner::IsGroupCleared() const
{
    return CurrentStatus == ESpawnGroupStatus::Cleared && GetAliveGhostCount() == 0;
}

void ASpawnGroupSpawner::ResetKilledCount()
{
    KilledCount = 0;
    TypeKilled.Empty();
}

TArray<AActor*> ASpawnGroupSpawner::GetSpawnedGhosts() const
{
    TArray<AActor*> Result;
    for (AActor* Ghost : SpawnedGhosts)
    {
        if (Ghost) Result.Add(Ghost);
    }
    return Result;
}

void ASpawnGroupSpawner::UpdateGroupStatus()
{
    const int32 Alive = GetAliveGhostCount();
    if (Alive == 0)
    {
        if (SpawnedCount == 0)
        {
            if (KilledCount > 0)
                CurrentStatus = ESpawnGroupStatus::Cleared;
            else
                CurrentStatus = ESpawnGroupStatus::Suppressed;
        }
        else
        {
            if (KilledCount + GetCapturedCount() == SpawnedCount)
                CurrentStatus = ESpawnGroupStatus::Cleared;
            else
                CurrentStatus = (KilledCount + GetCapturedCount() == 0) ? ESpawnGroupStatus::Active : ESpawnGroupStatus::PartiallyCleared;
        }
    }
    else
    {
        if (KilledCount + GetCapturedCount() == SpawnedCount)
            CurrentStatus = (KilledCount + GetCapturedCount() == 0) ? ESpawnGroupStatus::Inactive : ESpawnGroupStatus::Cleared;
        else
            CurrentStatus = (KilledCount + GetCapturedCount() == 0) ? ESpawnGroupStatus::Active : ESpawnGroupStatus::PartiallyCleared;
    }
}

void ASpawnGroupSpawner::OnGhostDestroyed(AActor* DestroyedActor)
{
    if (!DestroyedActor) return;

    FGuid ItemId;
    if (UFloorAssignmentComponent* Comp = DestroyedActor->FindComponentByClass<UFloorAssignmentComponent>())
    {
        ItemId = Comp->ItemId;

        for (auto Slot : AllSlots)
        {
            if (Slot.ItemId == ItemId)
            {
                if (Slot.State == EGhostState::Captured)
                {
                    return;
                }
            }
        }
    }

    if (ItemId.IsValid())
        // fixed
        // исправлено
        UpdateSlotState(ItemId, EGhostState::Killed);

    if (UClass* ActorClass = DestroyedActor->GetClass())
    {
        FName ClassName = ActorClass->GetFName();
        int32& CountRef = TypeKilled.FindOrAdd(ClassName);
        CountRef++;
    }

    SpawnedGhosts.Remove(Cast<AAlsCharacter>(DestroyedActor));
    OnGhostKilled.Broadcast(DestroyedActor, ESpawnGroupResolutionReason::Eliminated);
    KilledCount++;
    UpdateGroupStatus();
}

void ASpawnGroupSpawner::UpdateSlotState(const FGuid& ItemId, EGhostState NewState)
{
    for (FSpawnSlotState& Slot : AllSlots)
    {
        if (Slot.ItemId == ItemId)
        {
            if (Slot.State == EGhostState::Alive)
                Slot.State = NewState;
            break;
        }
    }
}

AActor* ASpawnGroupSpawner::SpawnSingleGhost(TSubclassOf<AActor> ActorClass, const FTransform& Transform)
{
    if (!ActorClass || !GetWorld()) return nullptr;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;

    AActor* Ghost = GetWorld()->SpawnActor<AActor>(ActorClass, Transform, SpawnParams);
    if (!Ghost)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Failed to spawn ghost of class %s"),
            *GetName(), *ActorClass->GetName());

        if(DebugSpawnPoints)
            DrawDebugSphere(GetWorld(), Transform.GetLocation(), 10.0f, 12, FColor::Red, false, 5.0f);

        return nullptr;
    }
    
    FGuid NewGuid;
	bool IsFoundGuid = false;

    for (auto& State : PendingEnemyStates)
    {
        if (State.ActorClass == ActorClass)
        {
            FGuid Id = State.ItemId;

			bool bFound = false;
            for (auto SG : SpawnedGhosts)
            {
				UFloorAssignmentComponent* AC = SG->GetComponentByClass<UFloorAssignmentComponent>();
                if (AC)
                {
                    if (AC->ItemId == Id)
                    {
						bFound = true;
						break;
                    }
				}
            }

            if (bFound)
            {
				continue;
            }

			NewGuid = Id;
            IsFoundGuid = true;
            break;
        }
    }
    
    if (UFloorAssignmentComponent* Component = Ghost->GetComponentByClass<UFloorAssignmentComponent>())
    {
        if (!Component->ItemId.IsValid())
            Component->ItemId = IsFoundGuid ? NewGuid : FGuid::NewGuid();

        Component->InteriorSetId = FloorAssignmentComp->InteriorSetId;
        Component->FloorId = FloorAssignmentComp->FloorId;
        Component->ActorType = EFloorActorType::SpawnItems;
        Component->SnapshotChannel = ESnapshotChannel::None;
        Component->GameplayTagContainer.AppendTags(EnemyGameplayTags);
    }
    else
    {
        UFloorAssignmentComponent* NewComp = NewObject<UFloorAssignmentComponent>(Ghost);
        NewComp->RegisterComponent();
        Ghost->AddInstanceComponent(NewComp);
        NewComp->ItemId = IsFoundGuid ? NewGuid : FGuid::NewGuid();
        NewComp->InteriorSetId = FloorAssignmentComp->InteriorSetId;
        NewComp->FloorId = FloorAssignmentComp->FloorId;
        NewComp->ActorType = EFloorActorType::SpawnItems;
        NewComp->SnapshotChannel = ESnapshotChannel::None;
        NewComp->GameplayTagContainer.AppendTags(EnemyGameplayTags);
    }

    if (const FSpawnedEnemyState* FoundState = FindStateForActor(Ghost))
    {
        //DeserializeAndApplyState(Ghost, *FoundState);
        //UE_LOG(LogTemp, Log, TEXT("SpawnGroupSpawner [%s]: Applied saved state for ItemId %s"),
        //    *GetName(), *Slot.ItemId.ToString());
    }

    Ghost->Tags.Append(EnemyTags);
    AddSpawnSlot(Ghost, Transform);
    OnGhostSpawned_BP(Ghost);
    return Ghost;
}

void ASpawnGroupSpawner::PublishGhostClearedEvent(ESpawnGroupResolutionReason Reason)
{
    if (!CachedEventBus.IsValid() || !SpawnGroupAsset) return;

    UEventBusSubsystem* EventBus = CachedEventBus.Get();
    if (!EventBus) return;

    UGhostClearedPayload* Payload = EventBus->CreatePayload<UGhostClearedPayload>();
    if (!Payload) return;

    FGuid MissionId;
    FGuid InteriorSetId;
    if (UFloorAssignmentComponent* MyFloorComp = FindComponentByClass<UFloorAssignmentComponent>())
        InteriorSetId = MyFloorComp->InteriorSetId;

    Payload->Setup(MissionId, SpawnGroupAsset->DisplayName.ToString(), InteriorSetId, RuntimeGroupId);

    FOutcomeEventBase Event;
    Event.OutcomeType = EOutcomeType::SpawnGroup;
    Event.OutcomeSpawnGroup = EOutcomeSpawnGroup::SpawnGroupCleared;
    Event.Payload = Payload;
    EventBus->PublishOutcome(Event);
}

FGuid ASpawnGroupSpawner::GetGroupId() const
{
    if (SpawnGroupAsset)
    {
        UFloorAssignmentComponent* Comp = FindComponentByClass<UFloorAssignmentComponent>();
        if (Comp && Comp->ItemId.IsValid())
            return Comp->ItemId;
        return FGuid::NewGuid();
    }
    return FGuid();
}

void ASpawnGroupSpawner::GatherSpawnLocationsFromChildren()
{
    SpawnLocations.Empty();
    TArray<AActor*> ChildrenArray;
    GetAttachedActors(ChildrenArray, true);
    for (AActor* Child : ChildrenArray)
    {
        if (ASpawnVolume* Volume = Cast<ASpawnVolume>(Child))
            SpawnLocations.Add(Volume);
    }

    if (SpawnLocations.Num() == 0)
    {
        UWorld* World = GetWorld();
        if (World)
        {
            for (TActorIterator<ASpawnVolume> It(World); It; ++It)
                SpawnLocations.Add(*It);
        }
    }
}

void ASpawnGroupSpawner::RestoreFromSlots(const TArray<FSpawnSlotState>& Slots)
{
    SafeDestroyAllGhosts();

    if (CurrentStatus == ESpawnGroupStatus::Suppressed || CurrentStatus == ESpawnGroupStatus::Inactive)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Group CurrentStatus Suppressed, skip spawn"), *GetName());
        return;
    }

    if (CurrentStatus == ESpawnGroupStatus::Cleared)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Group already Cleared, skip spawn"), *GetName());
        return;
    }

    if (!IsStoreSpawnParameters) return;

    if (!IsFullRespawn)
    {
        AllSlots = Slots;

        for (const FSpawnSlotState& Slot : AllSlots)
        {
            if (Slot.State != EGhostState::Alive) continue;
            if (!Slot.ActorClass) continue;

            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AActor* Ghost = GetWorld()->SpawnActor<AActor>(Slot.ActorClass, Slot.SpawnTransform, SpawnParams);
            if (!Ghost) continue;

            if (UFloorAssignmentComponent* Comp = Ghost->FindComponentByClass<UFloorAssignmentComponent>())
            {
                Comp->ItemId = Slot.ItemId;
                Comp->SnapshotChannel = ESnapshotChannel::None;
                Comp->ActorType = EFloorActorType::SpawnItems;
            }
            else
            {
                UFloorAssignmentComponent* NewComp = NewObject<UFloorAssignmentComponent>(Ghost);
                NewComp->RegisterComponent();
                Ghost->AddInstanceComponent(NewComp);
                NewComp->ItemId = Slot.ItemId;
                NewComp->SnapshotChannel = ESnapshotChannel::None;
                NewComp->ActorType = EFloorActorType::SpawnItems;
            }

            if (const FSpawnedEnemyState* FoundState = FindStateForActor(Ghost))
            {
                //DeserializeAndApplyState(Ghost, *FoundState);
                UE_LOG(LogTemp, Log, TEXT("SpawnGroupSpawner [%s]: Applied saved state for ItemId %s"),
                    *GetName(), *Slot.ItemId.ToString());
            }

            SpawnedGhosts.Add(Cast<AAlsCharacter>(Ghost));
            Ghost->OnDestroyed.AddDynamic(this, &ASpawnGroupSpawner::OnGhostDestroyed);
        }
        UpdateGroupStatus();
    }
    else
    {
        SpawnGroup();
    }
}

void ASpawnGroupSpawner::AddSpawnSlot(AActor* SpawnedActor, const FTransform& Transform)
{
    if (!SpawnedActor) return;

    FSpawnSlotState Slot;
    Slot.ItemId = FGuid::NewGuid();
    Slot.ActorClass = SpawnedActor->GetClass();
    Slot.SpawnTransform = Transform;
    Slot.State = EGhostState::Alive;

    if (UFloorAssignmentComponent* Comp = SpawnedActor->FindComponentByClass<UFloorAssignmentComponent>())
    {
        Slot.GameplayTags = Comp->GameplayTagContainer;
        Slot.TextTags = SpawnedActor->Tags;
        if (Comp->ItemId.IsValid())
            Slot.ItemId = Comp->ItemId;
    }

    AllSlots.Add(Slot);
}

void ASpawnGroupSpawner::MarkSlotDead(const FGuid& ItemId)
{
    for (FSpawnSlotState& Slot : AllSlots)
    {
        if (Slot.ItemId == ItemId)
        {
            Slot.State = EGhostState::Killed;
            break;
        }
    }
}

void ASpawnGroupSpawner::CaptureGhost(AActor* Ghost)
{
    if (!Ghost) return;
    FGuid ItemId;
    if (UFloorAssignmentComponent* Comp = Ghost->FindComponentByClass<UFloorAssignmentComponent>())
        ItemId = Comp->ItemId;
    if (!ItemId.IsValid()) return;

    UpdateSlotState(ItemId, EGhostState::Captured);

    if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        UGhostCapturedPayload* Payload = EventBus->CreatePayload<UGhostCapturedPayload>();
        Payload->Setup(Ghost);
        FOutcomeEventBase Ev;
        Ev.OutcomeType = EOutcomeType::SpawnGroup;
        Ev.OutcomeSpawnGroup = EOutcomeSpawnGroup::GhostCaptured;
        Ev.Payload = Payload;
        EventBus->PublishOutcome(Ev);
    }
}

void ASpawnGroupSpawner::RestoreFromStateWithoutCleanup(FSpawnGroupState& State)
{
    // Update group status
    // Обновляем статус группы
    CurrentStatus = State.Status;
    if (CurrentStatus == ESpawnGroupStatus::Cleared)
    {
        ClearGroup(State.ResolutionReason);
        SpawnedCount = 0;
        return;
    }

    // Update killed counters (for backward compatibility)
    // Обновляем счётчики убитых (для обратной совместимости)
    if (!State.bStoreSpawnParameters)
    {
        TypeKilled = State.TypeKilled;
        KilledCount = 0;
        for (const auto& Pair : TypeKilled) KilledCount += Pair.Value;
    }

    // If there are slots – restore ghosts without modifying AllSlots
    // Если есть слоты – восстанавливаем призраков, не изменяя AllSlots
    if (State.Slots.Num() > 0)
    {
        if (IsResetToAlive)
        {
            for (auto& Slot : State.Slots)
            {
                Slot.State = EGhostState::Alive;
            }
            IsResetToAlive = false;
        }

        SpawnedCount = AllSlots.Num();
        for (const FSpawnSlotState& Slot : State.Slots)
        {
            // Spawn only alive ones, and only if an actor with this ItemId does not exist in the world
            // Спавним только живых, и только если актор с таким ItemId отсутствует в мире
            if (Slot.State != EGhostState::Alive) continue;
            if (!Slot.ActorClass) continue;

            // Check if there is already an actor with this ItemId in the world
            // Проверяем, есть ли уже актор с таким ItemId в мире
            if (FindActorByItemId(Slot.ItemId))
                continue; // already exists – do not spawn again

            // If actor is missing – spawn it
            // Если актор отсутствует – спавним
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AActor* Ghost = GetWorld()->SpawnActor<AActor>(Slot.ActorClass, Slot.SpawnTransform, SpawnParams);
            if (!Ghost) continue;

            // Configure FloorAssignmentComponent
            // Настраиваем компонент FloorAssignmentComponent
            if (UFloorAssignmentComponent* Comp = Ghost->FindComponentByClass<UFloorAssignmentComponent>())
            {
                Comp->ItemId = Slot.ItemId;
                Comp->SnapshotChannel = ESnapshotChannel::None;
                Comp->ActorType = EFloorActorType::SpawnItems;
                Comp->GameplayTagContainer = Slot.GameplayTags;
                Ghost->Tags = Slot.TextTags;
            }
            else
            {
                UFloorAssignmentComponent* NewComp = NewObject<UFloorAssignmentComponent>(Ghost);
                NewComp->RegisterComponent();
                Ghost->AddInstanceComponent(NewComp);
                NewComp->ItemId = Slot.ItemId;
                NewComp->SnapshotChannel = ESnapshotChannel::None;
                NewComp->ActorType = EFloorActorType::SpawnItems;
                NewComp->GameplayTagContainer = Slot.GameplayTags;
                Ghost->Tags = Slot.TextTags;
            }

            if (const FSpawnedEnemyState* FoundState = FindStateForActor(Ghost))
            {
                DeserializeAndApplyState(Ghost, *FoundState);
                UE_LOG(LogTemp, Log, TEXT("SpawnGroupSpawner [%s]: Applied saved state for ItemId %s"),
                    *GetName(), *Slot.ItemId.ToString());
            }

            SpawnedGhosts.Add(Cast<AAlsCharacter>(Ghost));
            Ghost->OnDestroyed.AddDynamic(this, &ASpawnGroupSpawner::OnGhostDestroyed);
        }
        UpdateGroupStatus();
    }
}

FGuid ASpawnGroupSpawner::GetItemIdFromActor(AActor* Actor) const
{
    if (!IsValid(Actor)) return FGuid();
    if (UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>())
    {
        return Comp->ItemId;
    }
    return FGuid();
}

/*
void ASpawnGroupSpawner::ApplyEnemyStates(const TArray<FSpawnedEnemyState>& States)
{
    // Сохраняем состояния во временный массив, чтобы использовать при спавне
    // Можно сохранить в член класса, например, TMap<FGuid, FString> PendingStates
    // или просто хранить массив.
    PendingEnemyStates = States; // нужно добавить член класса
}
*/
FString ASpawnGroupSpawner::SerializeActorStateToJSON(AActor* Actor) const
{
    if (!IsValid(Actor)) return TEXT("{}");

    using namespace SpawnGroupSerialization;

    // 1. Собираем свойства самого актора
    TArray<FFloorSavedPropertyEntry> ActorProps;
    CollectSaveGameProperties(Actor, ActorProps);

    // 2. Собираем состояния всех компонентов
    TArray<FFloorSavedComponentState> ComponentStates;
    TArray<UActorComponent*> Components;
    Actor->GetComponents(Components);
    for (UActorComponent* C : Components)
    {
        if (!IsValid(C)) continue;
        FFloorSavedComponentState CState;
        CState.ComponentName = C->GetFName();
        CState.ComponentClassName = C->GetClass()->GetFName();
        CState.bWasActive = C->IsActive();
        CollectSaveGameProperties(C, CState.Properties);

        if (USceneComponent* SC = Cast<USceneComponent>(C))
        {
            if (USceneComponent* ParentSC = SC->GetAttachParent())
            {
                CState.bWasAttached = true;
                CState.AttachParentName = ParentSC->GetFName();
                CState.AttachSocketName = SC->GetAttachSocketName();
                CState.RelativeTransform = SC->GetRelativeTransform();
                CState.bHasRelativeTransform = true;
            }
            CState.WorldTransform = SC->GetComponentTransform();
            CState.bHasWorldTransform = true;
        }

        if (UPrimitiveComponent* PC = Cast<UPrimitiveComponent>(C))
        {
            CState.bWasSimulatingPhysics = PC->IsSimulatingPhysics();
            if (CState.bWasSimulatingPhysics)
            {
                CState.SavedLinearVelocity = PC->GetPhysicsLinearVelocity();
                CState.SavedAngularVelocityDeg = PC->GetPhysicsAngularVelocityInDegrees();
            }
        }

        ComponentStates.Add(MoveTemp(CState));
    }

    // 3. Строим корневой JSON-объект
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("ItemId"), GetItemIdFromActor(Actor).ToString());
    Root->SetStringField(TEXT("ActorClass"), Actor->GetClass()->GetPathName());
    Root->SetObjectField(TEXT("Transform"), TransformToJsonObject(Actor->GetActorTransform()));

    // Свойства актора
    TArray<TSharedPtr<FJsonValue>> ActorPropsArray;
    for (const auto& Prop : ActorProps)
        ActorPropsArray.Add(MakeShared<FJsonValueObject>(PropertyEntryToJson(Prop)));
    Root->SetArrayField(TEXT("ActorProperties"), ActorPropsArray);

    // Состояния компонентов
    TArray<TSharedPtr<FJsonValue>> CompArray;
    for (const auto& CS : ComponentStates)
        CompArray.Add(MakeShared<FJsonValueObject>(ComponentStateToJson(CS)));
    Root->SetArrayField(TEXT("ComponentStates"), CompArray);

    // 4. Сериализуем в строку
    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    return Output;
}
/*
void ASpawnGroupSpawner::DeserializeAndApplyState(AActor* Actor, const FSpawnedEnemyState& StateData) const
{
    if (!IsValid(Actor) || StateData.SerializedState.IsEmpty())
        return;

    // Парсим JSON
    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(StateData.SerializedState);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner: Failed to deserialize JSON for actor %s"), *Actor->GetName());
        return;
    }

    // ---- 1. Применяем свойства актора ----
    const TArray<TSharedPtr<FJsonValue>>* ActorPropsArray;
    if (Root->TryGetArrayField(TEXT("ActorProperties"), ActorPropsArray))
    {
        TArray<FFloorSavedPropertyEntry> Props;
        for (const auto& Val : *ActorPropsArray)
        {
            if (Val->Type == EJson::Object)
            {
                Props.Add(SpawnGroupSerialization::PropertyEntryFromJson(Val->AsObject()));
            }
        }
        SpawnGroupSerialization::ApplySaveGameProperties(Actor, Props);
    }

    // ---- 2. Восстанавливаем трансформацию актора (если есть) ----
    const TSharedPtr<FJsonObject>* TransformObj;
    if (Root->TryGetObjectField(TEXT("Transform"), TransformObj))
    {
        FTransform SavedTransform = SpawnGroupSerialization::TransformFromJsonObject(*TransformObj);
        Actor->SetActorTransform(SavedTransform, false, nullptr, ETeleportType::TeleportPhysics);
    }

    // ---- 3. Применяем состояния компонентов ----
    const TArray<TSharedPtr<FJsonValue>>* CompArray;
    if (Root->TryGetArrayField(TEXT("ComponentStates"), CompArray))
    {
        // Строим карту компонентов по имени для быстрого доступа
        TMap<FName, UActorComponent*> ComponentsByName;
        for (UActorComponent* C : Actor->GetComponents())
        {
            if (IsValid(C))
                ComponentsByName.Add(C->GetFName(), C);
        }

        // Сначала восстанавливаем аттачмент (привязку) для компонентов с bWasAttached == true
        // Это нужно делать до установки трансформаций, чтобы относительные трансформы считались корректно.
        for (const auto& Val : *CompArray)
        {
            if (Val->Type != EJson::Object) continue;
            FFloorSavedComponentState CState = SpawnGroupSerialization::ComponentStateFromJson(Val->AsObject());
            if (!CState.bWasAttached) continue;

            UActorComponent** Found = ComponentsByName.Find(CState.ComponentName);
            if (!Found || !IsValid(*Found)) continue;
            USceneComponent* TargetSC = Cast<USceneComponent>(*Found);
            if (!TargetSC) continue;

            UActorComponent** ParentFound = ComponentsByName.Find(CState.AttachParentName);
            if (ParentFound && IsValid(*ParentFound))
            {
                if (USceneComponent* ParentSC = Cast<USceneComponent>(*ParentFound))
                {
                    TargetSC->AttachToComponent(ParentSC, FAttachmentTransformRules::KeepRelativeTransform, CState.AttachSocketName);
                }
            }
        }

        // Теперь восстанавливаем свойства и трансформы всех компонентов
        for (const auto& Val : *CompArray)
        {
            if (Val->Type != EJson::Object) continue;
            FFloorSavedComponentState CState = SpawnGroupSerialization::ComponentStateFromJson(Val->AsObject());

            UActorComponent** Found = ComponentsByName.Find(CState.ComponentName);
            if (!Found || !IsValid(*Found)) continue;
            UActorComponent* Target = *Found;

            // Восстанавливаем SaveGame-свойства компонента
            SpawnGroupSerialization::ApplySaveGameProperties(Target, CState.Properties);

            // Восстанавливаем активность
            if (Target->IsActive() != CState.bWasActive)
            {
                if (CState.bWasActive)
                    Target->Activate(true);
                else
                    Target->Deactivate();
            }

            // Восстанавливаем трансформацию для SceneComponent
            if (USceneComponent* SC = Cast<USceneComponent>(Target))
            {
                SpawnGroupSerialization::RestoreSceneComponentTransform(SC, CState);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("SpawnGroupSpawner: Applied saved state to actor %s"), *Actor->GetName());
}
*/
const FSpawnedEnemyState* ASpawnGroupSpawner::FindStateForActor(AActor* Actor) const
{
    if (!IsValid(Actor)) return nullptr;
    FGuid Id = GetItemIdFromActor(Actor);
    if (!Id.IsValid()) return nullptr;
    for (const FSpawnedEnemyState& State : PendingEnemyStates)
    {
        if (State.ItemId == Id)
            return &State;
    }
    return nullptr;
}

void ASpawnGroupSpawner::CaptureEnemyStates(TArray<FSpawnedEnemyState>& OutStates)
{
    OutStates.Empty();
    for (AActor* Ghost : SpawnedGhosts)
    {
        if (!IsValid(Ghost)) continue;
        FSpawnedEnemyState State;
        State.ItemId = GetItemIdFromActor(Ghost);
        if (!State.ItemId.IsValid())
            continue;
        State.ActorClass = Ghost->GetClass();
        State.SpawnTransform = Ghost->GetActorTransform();
        State.SerializedState = SerializeActorStateToJSON(Ghost);
        OutStates.Add(State);
    }
}

void ASpawnGroupSpawner::RestoreEnemyStates(const TArray<FSpawnedEnemyState>& InStates)
{
    if (IsSavingHealth)
    {
        PendingEnemyStates = InStates;
    }
}

AActor* ASpawnGroupSpawner::FindActorByItemId(const FGuid& ItemId) const
{
    if (!GetWorld()) return nullptr;
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor)) continue;
        if (UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>())
        {
            if (Comp->ItemId == ItemId)
                return Actor;
        }
    }
    return nullptr;
}

int32 ASpawnGroupSpawner::GetCapturedCount() const
{
    int32 Count = 0;
    for (const FSpawnSlotState& Slot : AllSlots)
        if (Slot.State == EGhostState::Captured)
            Count++;
    return Count;
}

void ASpawnGroupSpawner::SetAliveAllGhosts()
{
    for (auto& Slot : AllSlots)
    {
        Slot.State = EGhostState::Alive;
    }
}

void ASpawnGroupSpawner::DeserializeAndApplyState(AActor* Actor, const FSpawnedEnemyState& StateData) const
{
    if (!IsValid(Actor) || StateData.SerializedState.IsEmpty())
        return;

    // Парсим JSON
    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(StateData.SerializedState);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
        return;

    // 1. Применяем свойства актора
    const TArray<TSharedPtr<FJsonValue>>* ActorPropsArray;
    if (Root->TryGetArrayField(TEXT("ActorProperties"), ActorPropsArray))
    {
        TArray<FFloorSavedPropertyEntry> Props;
        for (const auto& Val : *ActorPropsArray)
        {
            if (Val->Type == EJson::Object)
            {
                TSharedPtr<FJsonObject> Obj = Val->AsObject();
                FFloorSavedPropertyEntry Entry;
                Entry.PropertyName = FName(*Obj->GetStringField(TEXT("PropertyName")));
                Entry.ValueText = Obj->GetStringField(TEXT("ValueText"));
                Props.Add(Entry);
            }
        }
        // Применяем к актору
        SpawnGroupSerialization::ApplySaveGameProperties(Actor, Props);
    }

    // 2. Применяем свойства компонентов
    const TArray<TSharedPtr<FJsonValue>>* CompArray;
    if (Root->TryGetArrayField(TEXT("ComponentStates"), CompArray))
    {
        // Строим карту компонентов по имени для быстрого доступа
        TMap<FName, UActorComponent*> ComponentsByName;
        for (UActorComponent* C : Actor->GetComponents())
            if (IsValid(C))
                ComponentsByName.Add(C->GetFName(), C);

        for (const auto& Val : *CompArray)
        {
            if (Val->Type != EJson::Object) continue;
            TSharedPtr<FJsonObject> Obj = Val->AsObject();

            // Восстанавливаем FFloorSavedComponentState из JSON
            FFloorSavedComponentState CS;
            CS.ComponentName = FName(*Obj->GetStringField(TEXT("ComponentName")));
            // ... можно заполнить остальные поля, но для применения свойств достаточно имени и Properties
            if (Obj->HasField(TEXT("Properties")))
            {
                const TArray<TSharedPtr<FJsonValue>>& PropsArray = Obj->GetArrayField(TEXT("Properties"));
                for (const auto& PropVal : PropsArray)
                {
                    if (PropVal->Type == EJson::Object)
                    {
                        TSharedPtr<FJsonObject> PropObj = PropVal->AsObject();
                        FFloorSavedPropertyEntry Entry;
                        Entry.PropertyName = FName(*PropObj->GetStringField(TEXT("PropertyName")));
                        Entry.ValueText = PropObj->GetStringField(TEXT("ValueText"));
                        CS.Properties.Add(Entry);
                    }
                }
            }

            // Применяем к найденному компоненту
            if (UActorComponent** Found = ComponentsByName.Find(CS.ComponentName))
            {
                SpawnGroupSerialization::ApplySaveGameProperties(*Found, CS.Properties);
            }
        }
    }

    // 3. (Опционально) Восстановление трансформа – если нужно, можно применить Transform из StateData,
    // но мы обычно оставляем трансформ, который задан при спавне.
    // Если нужно восстановить трансформ из JSON, можно раскомментировать:
    // if (Root->HasField(TEXT("Transform")))
    // {
    //     FTransform SavedTransform = TransformFromJsonObject(Root->GetObjectField(TEXT("Transform")));
    //     Actor->SetActorTransform(SavedTransform);
    // }
}


#if WITH_EDITOR
void ASpawnGroupSpawner::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (GetWorld() && !GetWorld()->IsGameWorld())
        DrawEditorLines();
}

void ASpawnGroupSpawner::DrawEditorLines() const
{
    if (SpawnLocations.Num() == 0) return;
    const FVector Start = GetActorLocation();
    for (ASpawnVolume* Volume : SpawnLocations)
    {
        if (IsValid(Volume))
            DrawDebugLine(GetWorld(), Start, Volume->GetActorLocation(), FColor::Green, false, 0.0f, 0, 2.0f);
    }
}
#endif