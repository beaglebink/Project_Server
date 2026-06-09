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
#include "FloorAssignmentComponent.h"

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
        SpawnRotation = Volume->GetDefaultRotation();
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
    if (BlockNewSpawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Mission New Stage Disable Spawn"), *GetName());
        return;
    }

    SpawnGroup();
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

    // 1. Желаемое количество каждого класса
    TMap<UClass*, int32> DesiredCounts;
    if (SpawnGroupAsset->Composition.bUsePool)
    {
        for (const FSpawnTypeCount& TypeCount : SpawnGroupAsset->Composition.ActorsPool)
        {
            DesiredCounts.Add(TypeCount.ActorClass, TypeCount.Count);
        }
    }
    else
    {
        int32 DesiredCount = SpawnGroupAsset->Composition.Count;
        TArray<TSubclassOf<AAlsCharacter>> ClassesToSpawn = SpawnGroupAsset->Composition.ActorClasses;

        for (int32 i = 0; i < DesiredCount; ++i)
        //for (TSubclassOf<AActor> Class : SpawnGroupAsset->Composition.ActorClasses)
        {
            //FinalClasses.Add(ClassesToSpawn[i % ClassesToSpawn.Num()]);
            UClass* Class = ClassesToSpawn[i % ClassesToSpawn.Num()];
            if (Class)
                DesiredCounts.FindOrAdd(Class)++;
        }
    }

    // 2. Вычитаем уже убитых (TypeKilled)
    TArray<TSubclassOf<AActor>> ClassesToSpawn;
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

    if (ClassesToSpawn.Num() == 0)
    {
        // Все враги уже убиты
        CurrentStatus = ESpawnGroupStatus::Cleared;
        OnAllCleared.Broadcast(ESpawnGroupResolutionReason::Eliminated);
        return;
    }

    // Очищаем предыдущих призраков (если есть)
    for (AActor* Ghost : SpawnedGhosts)
    {
        if (Ghost)
        {
            Ghost->OnDestroyed.RemoveDynamic(this, &ASpawnGroupSpawner::OnGhostDestroyed);
            Ghost->Destroy();
        }
    }
    SpawnedGhosts.Empty();
    SpawnedCount = 0;
    bHasPublishedClear = false;

    CurrentStatus = ESpawnGroupStatus::Active;
    RuntimeGroupId = SpawnGroupAsset->GroupId;

    // Таймер последовательного спавна
    FTimerDelegate Delegate = FTimerDelegate::CreateLambda([this, ClassesToSpawn, World]()
        {
            if (SpawnedCount >= ClassesToSpawn.Num())
            {
                World->GetTimerManager().ClearTimer(TimerHandle);
                OnAllSpawned.Broadcast();
                BlockNewSpawn = true;
                UE_LOG(LogTemp, Log, TEXT("SpawnGroupSpawner [%s]: Spawned %d ghosts"), *GetName(), SpawnedGhosts.Num());
                return;
            }

            ASpawnVolume* Location = GetRandomSpawnLocation();
            const FTransform SpawnTransform = GetTransformFromLocation(Location);
            AActor* Ghost = SpawnSingleGhost(ClassesToSpawn[SpawnedCount], SpawnTransform);
            if (Ghost)
            {
                SpawnedCount++;
                SpawnedGhosts.Add(Cast<AAlsCharacter>(Ghost));
                Ghost->OnDestroyed.AddDynamic(this, &ASpawnGroupSpawner::OnGhostDestroyed);
                OnGhostSpawned.Broadcast(Ghost);
            }
        });

    World->GetTimerManager().SetTimer(TimerHandle, Delegate, SpawnInterval, true);
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
        UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSpawner [%s]: Failed to spawn ghost of class %s"),
            *GetName(), *ActorClass->GetName());
        return nullptr;
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
        RuntimeGroupId.Id
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