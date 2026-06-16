// SpawnGroupSpawner.cpp
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
        // InteriorSetId, FloorId, ItemId будут заполнены в редакторе или автоматически при спавне
    }
}

void ASpawnGroupSpawner::BeginPlay()
{
    Super::BeginPlay();

    // Убеждаемся, что есть FloorAssignmentComponent
    UFloorAssignmentComponent* FloorComp = FindComponentByClass<UFloorAssignmentComponent>();
    if (!FloorComp)
    {
        FloorComp = NewObject<UFloorAssignmentComponent>(this);
        FloorComp->RegisterComponent();
    }

    // Генерируем ItemId, если отсутствует
    if (!FloorComp->ItemId.IsValid())
    {
        FloorComp->ItemId = FGuid::NewGuid();
    }
    FloorComp->SnapshotChannel = ESnapshotChannel::Snapshot;
    FloorComp->ActorType = EFloorActorType::SpawnGroupSpawner;

    // Регистрация через EventBus
    if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        if (SpawnGroupAsset)
        {
            USpawnGroupRegistrationPayload* Payload = EventBus->CreatePayload<USpawnGroupRegistrationPayload>();
            Payload->Setup(FloorComp->ItemId, SpawnGroupAsset->GroupId);

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
    // Дерегистрация через EventBus
    if (UFloorAssignmentComponent* FloorComp = FindComponentByClass<UFloorAssignmentComponent>())
    {
        if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
        {
            if (SpawnGroupAsset)
            {
                USpawnGroupRegistrationPayload* Payload = EventBus->CreatePayload<USpawnGroupRegistrationPayload>();
                Payload->Setup(FloorComp->ItemId, SpawnGroupAsset->GroupId);

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
    for(AActor* Ghost : SpawnedGhosts)
    {
        if (!IsValid(Ghost)) continue;
        StoredSpawnParameters.Add(Ghost->GetClass(), Ghost->GetActorTransform());
	}
}

ASpawnVolume* ASpawnGroupSpawner::GetRandomSpawnLocation() const
{
    if (SpawnLocations.Num() == 0)
    {
        return nullptr;
    }
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
        // Нет объёмов – спавним на позиции спавнера
        SpawnLocation = GetActorLocation();
    }

    SpawnLocation += SpawnOffset;
    return FTransform(SpawnRotation, SpawnLocation, FVector::OneVector);
}

void ASpawnGroupSpawner::SpawnGroupInternal()
{
    /*
    if (BlockNewSpawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Mission New Stage Disable Spawn"), *GetName());
        return;
    }
    */
    //IsRestored = false;
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
        Slot.bIsAlive = true;

        // Опционально: сериализация SaveGame-свойств (пока пропущено)
        Slots.Add(Slot);
    }
    return Slots;
}

void ASpawnGroupSpawner::RestoreFromState(const FSpawnGroupState& State)
{
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

    TypeKilled = State.TypeKilled;
    KilledCount = 0;
    for (const auto& Pair : TypeKilled) KilledCount += Pair.Value;

    CurrentStatus = State.Status;
    if (CurrentStatus == ESpawnGroupStatus::Cleared)
        ClearGroup(State.ResolutionReason);

    SpawnedCount = 0;
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

    if (CurrentStatus == ESpawnGroupStatus::Cleared)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Group already Cleared, skip spawn"), *GetName());
        return;
    }

    if (SpawnGroupAsset->Composition.bUsePool)
    {
        // 1. Желаемое количество каждого класса
        TMap<UClass*, int32> DesiredCounts;

        for (const FSpawnTypeCount& TypeCount : SpawnGroupAsset->Composition.ActorsPool)
        {
            DesiredCounts.Add(TypeCount.ActorClass, TypeCount.Count);
        }

        TArray<TSubclassOf<AActor>> ClassesToSpawn;

        // 2. Вычитаем уже убитых (TypeKilled)

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

        FTimerDelegate Delegate = FTimerDelegate::CreateLambda([this, ClassesToSpawn, DesiredCounts, World]()
            {
                ASpawnVolume* Location = GetRandomSpawnLocation();

                if (ClassesToSpawn.Num() <= 0 || !ClassesToSpawn.IsValidIndex(SpawnedCount))
                {
                    UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Invalid index for FinalClasses"), *GetName());
                    World->GetTimerManager().ClearTimer(TimerHandle);
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
                    SpawnedGhosts.Add(Cast<AAlsCharacter>(Ghost));
                    Ghost->OnDestroyed.AddDynamic(this, &ASpawnGroupSpawner::OnGhostDestroyed);

                    OnGhostSpawned.Broadcast(Ghost);
                }

                if (SpawnGroupAsset->Composition.Count <= SpawnedCount - KilledCount)
                {
                    KilledCount = DesiredCounts.Num();
                    World->GetTimerManager().ClearTimer(TimerHandle);
                    UE_LOG(LogTemp, Log, TEXT("SpawnGroupSpawner [%s]: Spawned %d ghosts"),
                        *GetName(), SpawnedGhosts.Num());
                    return;
                }

            });

        CurrentStatus = ESpawnGroupStatus::Active;

        World->GetTimerManager().SetTimer(TimerHandle, Delegate, SpawnInterval, true);
    }
    else
    {
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

        // Если указано больше Count, чем классов, циклически повторяем классы
        TArray<TSubclassOf<AActor>> FinalClasses;
        for (int32 i = 0; i < DesiredCount; ++i)
        {
            FinalClasses.Add(ClassesToSpawn[i % ClassesToSpawn.Num()]);
        }

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
                KilledCount = DesiredCount;
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
                SpawnedGhosts.Add(Cast<AAlsCharacter>(Ghost));
                Ghost->OnDestroyed.AddDynamic(this, &ASpawnGroupSpawner::OnGhostDestroyed);

                OnGhostSpawned.Broadcast(Ghost);
            }

            if (DesiredCount <= KilledCount)
            {
                KilledCount = DesiredCount;
                World->GetTimerManager().ClearTimer(TimerHandle);
                return;
            }

            if (SpawnedCount >= DesiredCount - KilledCount)
            {
                // Все призраки заспавнены
                World->GetTimerManager().ClearTimer(TimerHandle);
                OnAllSpawned.Broadcast();

                UE_LOG(LogTemp, Log, TEXT("SpawnGroupSpawner [%s]: Spawned %d ghosts"),
                    *GetName(), SpawnedGhosts.Num());
            }
        });

        CurrentStatus = ESpawnGroupStatus::Active;

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

    // Уничтожаем всех призраков
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

    // Уведомить Blueprint
    OnAllCleared.Broadcast(Reason);

    if (bDestroyOnClear)
    {
        Destroy();
    }

    CurrentStatus = ESpawnGroupStatus::Cleared;
}

void ASpawnGroupSpawner::ResetGroup()
{
    if (!bAllowRespawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Reset not allowed (bAllowRespawn=false)"), *GetName());
        return;
    }

    // Сбрасываем статистику убитых
    TypeKilled.Empty();
    KilledCount = 0;
    SpawnedCount = 0;

    ClearGroup(ESpawnGroupResolutionReason::Other);
    SpawnGroup();
}

int32 ASpawnGroupSpawner::GetAliveGhostCount() const
{
    int32 Alive = 0;
    for (AActor* Ghost : SpawnedGhosts)
    {
        if (IsValid(Ghost))
        {
            Alive++;
        }
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
        if (Ghost)
        {
            Result.Add(Ghost);
        }
    }
    return Result;
}

void ASpawnGroupSpawner::UpdateGroupStatus()
{
    const int32 Alive = GetAliveGhostCount();
    if (Alive == 0)
    {
        // Проверяем, все ли запланированные враги убиты (через TypeKilled)
        bool bAllKilled = false;
        // (опциональная проверка, можно оставить только по Alive)
        // Если нет живых, но ещё не все убиты, то не переводим в Cleared.
        // Для простоты переводим в Cleared только при Alive == 0 и если SpawnedGhosts пуст.
        if (SpawnedGhosts.Num() == 0)
        {
            ClearGroup(ESpawnGroupResolutionReason::Eliminated);
        }
    }
    else if (Alive > 0 && KilledCount > 0 && CurrentStatus == ESpawnGroupStatus::Active)
    {
        CurrentStatus = ESpawnGroupStatus::PartiallyCleared;
    }
}

void ASpawnGroupSpawner::OnGhostDestroyed(AActor* DestroyedActor)
{
    if (DestroyedActor)
    {
        UClass* ActorClass = DestroyedActor->GetClass();
        if (ActorClass)
        {
            FName ClassName = ActorClass->GetFName();
            int32& CountRef = TypeKilled.FindOrAdd(ClassName);
            CountRef++;
        }

        OnGhostKilled.Broadcast(DestroyedActor, ESpawnGroupResolutionReason::Eliminated);
        SpawnedGhosts.Remove(Cast<AAlsCharacter>(DestroyedActor));
    }

    KilledCount++;
    UpdateGroupStatus();
}

AActor* ASpawnGroupSpawner::SpawnSingleGhost(TSubclassOf<AActor> ActorClass, const FTransform& Transform)
{
    if (!ActorClass || !GetWorld())
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    AActor* Ghost = GetWorld()->SpawnActor<AActor>(ActorClass, Transform, SpawnParams);
    if (!Ghost)
    {
		//DrawDebugSphere(GetWorld(), Transform.GetLocation(), 50.0f, 12, FColor::Red, false, 20.0f);
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Failed to spawn ghost of class %s"),
            *GetName(), *ActorClass->GetName());
        return nullptr;
    }

    UFloorAssignmentComponent* Component = Ghost->GetComponentByClass<UFloorAssignmentComponent>();
    if (Component)
    {
        if (!Component->ItemId.IsValid())
        {
            Component->ItemId = FGuid::NewGuid();
        }

        Component->InteriorSetId = FloorAssignmentComp->InteriorSetId;
        Component->FloorId = FloorAssignmentComp->FloorId;
		Component->SnapshotChannel = ESnapshotChannel::None;
    }

    return Ghost;
}

void ASpawnGroupSpawner::PublishGhostClearedEvent(ESpawnGroupResolutionReason Reason)
{
    if (!CachedEventBus.IsValid() || !SpawnGroupAsset)
    {
        return;
    }

    UEventBusSubsystem* EventBus = CachedEventBus.Get();
    if (!EventBus) return;

    UGhostClearedPayload* Payload = EventBus->CreatePayload<UGhostClearedPayload>();
    if (!Payload) return;

    // TODO: получить активный MissionId из MissionSubsystem
    FGuid MissionId; // пока пустой

    // Получаем InteriorSetId из текущего контекста (через FloorAssignmentComponent спавнера? упростим)
    FGuid InteriorSetId;
    if (UFloorAssignmentComponent* MyFloorComp = FindComponentByClass<UFloorAssignmentComponent>())
    {
        InteriorSetId = MyFloorComp->InteriorSetId;
    }

    Payload->Setup(
        MissionId,
        SpawnGroupAsset->DisplayName.ToString(),
        InteriorSetId,
        RuntimeGroupId
    );

    FOutcomeEventBase Event;
    Event.OutcomeType = EOutcomeType::SpawnGroup;
    Event.OutcomeSpawnGroup = EOutcomeSpawnGroup::SpawnGroupCleared;
    Event.Payload = Payload;

    EventBus->PublishOutcome(Event);
}

void ASpawnGroupSpawner::GatherSpawnLocationsFromChildren()
{
    SpawnLocations.Empty();
    TArray<AActor*> ChildrenArray;
    GetAttachedActors(ChildrenArray, true);
    for (AActor* Child : ChildrenArray)
    {
        if (ASpawnVolume* Volume = Cast<ASpawnVolume>(Child))
        {
            SpawnLocations.Add(Volume);
        }
    }

    // Если дочерних нет – опционально ищем все ASpawnVolume на уровне
    if (SpawnLocations.Num() == 0)
    {
        UWorld* World = GetWorld();
        if (World)
        {
            for (TActorIterator<ASpawnVolume> It(World); It; ++It)
            {
                SpawnLocations.Add(*It);
            }
        }
    }
}

void ASpawnGroupSpawner::RestoreFromSlots(const TArray<FSpawnSlotState>& Slots)
{
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

    if (Restore)
    {
        Restore = false;
        SpawnGroup();
        return;
    }

    if (!IsStoreSpawnParameters) return;

    // Очищаем старых призраков
    ClearGroup(ESpawnGroupResolutionReason::Other);

    // Спавним новых по слотам
    for (const FSpawnSlotState& Slot : Slots)
    {
        if (!Slot.ActorClass) continue;

        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AActor* Ghost = GetWorld()->SpawnActor<AActor>(Slot.ActorClass, FTransform(Slot.SpawnTransform.GetRotation(), Slot.SpawnTransform.GetLocation(), FVector(1.0, 1.0, 1.0)), SpawnParams);
        if (!Ghost) continue;

        // Принудительно устанавливаем ItemId в компоненте
        if (UFloorAssignmentComponent* Comp = Ghost->FindComponentByClass<UFloorAssignmentComponent>())
        {
            Comp->ItemId = Slot.ItemId;
            Comp->SnapshotChannel = ESnapshotChannel::None;
            Comp->ActorType = EFloorActorType::SpawnItems; // или другой тип
        }
        else
        {
            // Если компонента нет, создаём его (чтобы спавнер мог отслеживать призрака)
            UFloorAssignmentComponent* NewComp = NewObject<UFloorAssignmentComponent>(Ghost);
            NewComp->RegisterComponent();
            Ghost->AddInstanceComponent(NewComp);
            NewComp->ItemId = Slot.ItemId;
            NewComp->SnapshotChannel = ESnapshotChannel::None;
            NewComp->ActorType = EFloorActorType::SpawnItems;
        }

        SpawnedGhosts.Add(Cast<AAlsCharacter>(Ghost));
        Ghost->OnDestroyed.AddDynamic(this, &ASpawnGroupSpawner::OnGhostDestroyed);
        OnGhostSpawned.Broadcast(Ghost);
    }

    CurrentStatus = ESpawnGroupStatus::Active;
    SpawnedCount = SpawnedGhosts.Num();
    TypeKilled.Empty();
    KilledCount = 0;
}

#if WITH_EDITOR
void ASpawnGroupSpawner::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // Рисуем линии только в редакторе и не в игре
    if (GetWorld() && !GetWorld()->IsGameWorld())
    {
        DrawEditorLines();
    }
}

void ASpawnGroupSpawner::DrawEditorLines() const
{
    if (SpawnLocations.Num() == 0) return;

    const FVector Start = GetActorLocation();
    for (ASpawnVolume* Volume : SpawnLocations)
    {
        if (IsValid(Volume))
        {
            const FVector End = Volume->GetActorLocation();
            DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 0.0f, 0, 2.0f);
        }
    }
}
#endif