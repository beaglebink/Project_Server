#include "InteriorObjectCheckCondition.h"
#include "FloorAssignmentComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "InteriorSubsystem.h"
#include "CheckCoordinatorComponent.h"

static FString GetActorNameFromSoftPtr(const TSoftObjectPtr<AActor>& SoftPtr)
{
    if (!SoftPtr.IsValid())
        return FString();
    FString Path = SoftPtr.ToString();
    int32 LastDot = -1;
    Path.FindLastChar('.', LastDot);
    if (LastDot != INDEX_NONE)
        return Path.Mid(LastDot + 1);
    return Path;
}

// ----- UInteriorObjectExistsCondition -----

void UInteriorObjectExistsCondition::Initialize(UCheckCoordinatorComponent* InCoordinator, UEventBusSubsystem* InEventBus)
{
    Super::Initialize(InCoordinator, InEventBus);
    CacheActorInfo();
}

void UInteriorObjectExistsCondition::CacheActorInfo()
{
    // Пытаемся получить актор и сохранить имя и ItemId
    AActor* Actor = ActorRef.Get();
    if (IsValid(Actor))
    {
        CachedActorName = Actor->GetName();
        if (UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>())
        {
            ItemId = Comp->ItemId;
            bItemIdCached = true;
        }
        return;
    }

    // Если актор невалиден, но у нас есть InitialItemId, сохраняем его, имя пока не знаем
    if (InitialItemId.IsValid() && !bItemIdCached)
    {
        ItemId = InitialItemId;
        bItemIdCached = true;
    }

    // Если CachedActorName всё ещё пуст, попытаемся извлечь имя из мягкой ссылки
    if (CachedActorName.IsEmpty() && ActorRef.IsValid())
    {
        CachedActorName = GetActorNameFromSoftPtr(ActorRef);
    }
}

void UInteriorObjectExistsCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    UWorld* World = GetWorld();
    if (!World)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no world)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    bool bFound = false;
    AActor* Actor = ActorRef.Get();

    if (IsValid(Actor) && Actor->GetWorld() == World)
    {
        bFound = true;
        // Если имя ещё не сохранено или актор изменился, обновим (но вряд ли)
        if (CachedActorName.IsEmpty())
            CachedActorName = Actor->GetName();
        if (!bItemIdCached)
        {
            if (UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>())
            {
                ItemId = Comp->ItemId;
                bItemIdCached = true;
            }
        }
    }

    // Если не удалось получить ItemId, используем InitialItemId (если задан)
    if (!bItemIdCached && InitialItemId.IsValid())
    {
        ItemId = InitialItemId;
        bItemIdCached = true;
    }

    bApproved = (bFound == bShouldExist);
    bCompleted = true;
    /*
    FString LogId;
    if (!CachedActorName.IsEmpty())
        LogId = CachedActorName;
    else if (ItemId.IsValid())
        LogId = FString::Printf(TEXT("ItemId:%s"), *ItemId.ToString());
    else
        LogId = TEXT("Unknown");
    */
    LogVerbose(FString::Printf(TEXT("→ %s"),
        bApproved ? TEXT("true") : TEXT("false")), false);
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorObjectExistsCondition::GetDescription() const
{
    if (!CachedActorName.IsEmpty())
    {
        return FString::Printf(TEXT("Object '%s' exists: %s"), *CachedActorName, bShouldExist ? TEXT("true") : TEXT("false"));
    }

    if (ItemId.IsValid())
    {
        if (UWorld* World = GetWorld())
        {
            if (UGameInstance* GI = World->GetGameInstance())
            {
                if (UInteriorSubsystem* InteriorSub = GI->GetSubsystem<UInteriorSubsystem>())
                {
                    FFloorPopulationRecord Rec;
                    /*
                    if (InteriorSub->FindPopulationRecordByItemId(ItemId, Rec))
                    {
                        FString Name = Rec.ActorInstanceName.IsEmpty() ? TEXT("Unnamed") : Rec.ActorInstanceName;
                        FString ClassName = Rec.SourceClass ? Rec.SourceClass->GetName() : TEXT("UnknownClass");
                        return FString::Printf(TEXT("Object '%s' (Class: %s, ItemId: %s) exists: %s"),
                            *Name, *ClassName, *ItemId.ToString(), bShouldExist ? TEXT("true") : TEXT("false"));
                    }
                    */
                }
            }
        }
        return FString::Printf(TEXT("Object (ItemId: %s) exists: %s"), *ItemId.ToString(), bShouldExist ? TEXT("true") : TEXT("false"));
    }

    AActor* Actor = ActorRef.Get();
    FString Name = IsValid(Actor) ? Actor->GetName() : TEXT("Unknown");
    return FString::Printf(TEXT("Object %s exists: %s"), *Name, bShouldExist ? TEXT("true") : TEXT("false"));
}

// ----- UInteriorObjectDestroyedCondition -----

void UInteriorObjectDestroyedCondition::Initialize(UCheckCoordinatorComponent* InCoordinator, UEventBusSubsystem* InEventBus)
{
    Super::Initialize(InCoordinator, InEventBus);
    CacheActorInfo();
}

void UInteriorObjectDestroyedCondition::CacheActorInfo()
{
    AActor* Actor = ActorRef.Get();
    if (IsValid(Actor))
    {
        CachedActorName = Actor->GetName();
        if (UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>())
        {
            ItemId = Comp->ItemId;
            bItemIdCached = true;
        }
        return;
    }

    if (InitialItemId.IsValid() && !bItemIdCached)
    {
        ItemId = InitialItemId;
        bItemIdCached = true;
    }

    if (CachedActorName.IsEmpty() && ActorRef.IsValid())
    {
        CachedActorName = GetActorNameFromSoftPtr(ActorRef);
    }
}

void UInteriorObjectDestroyedCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    UWorld* World = GetWorld();
    if (!World)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no world)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    bool bExists = false;
    AActor* Actor = ActorRef.Get();

    if (IsValid(Actor) && Actor->GetWorld() == World)
    {
        bExists = true;
        if (CachedActorName.IsEmpty())
            CachedActorName = Actor->GetName();
        if (!bItemIdCached)
        {
            if (UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>())
            {
                ItemId = Comp->ItemId;
                bItemIdCached = true;
            }
        }
    }

    if (!bItemIdCached && InitialItemId.IsValid())
    {
        ItemId = InitialItemId;
        bItemIdCached = true;
    }

    bool bIsDestroyed = !bExists;
    bApproved = (bIsDestroyed == bShouldBeDestroyed);
    bCompleted = true;

    FString LogId;
    if (!CachedActorName.IsEmpty())
        LogId = CachedActorName;
    else if (ItemId.IsValid())
        LogId = FString::Printf(TEXT("ItemId:%s"), *ItemId.ToString());
    else
        LogId = TEXT("Unknown");

    LogVerbose(FString::Printf(TEXT("→ %s (object: %s)"),
        bApproved ? TEXT("true") : TEXT("false"),
        *LogId), false);
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorObjectDestroyedCondition::GetDescription() const
{
    if (!CachedActorName.IsEmpty())
    {
        return FString::Printf(TEXT("Object '%s' destroyed: %s"), *CachedActorName, bShouldBeDestroyed ? TEXT("true") : TEXT("false"));
    }

    if (ItemId.IsValid())
    {
        if (UWorld* World = GetWorld())
        {
            if (UGameInstance* GI = World->GetGameInstance())
            {
                if (UInteriorSubsystem* InteriorSub = GI->GetSubsystem<UInteriorSubsystem>())
                {
                    FFloorPopulationRecord Rec;
                    /*
                    if (InteriorSub->FindPopulationRecordByItemId(ItemId, Rec))
                    {
                        FString Name = Rec.ActorInstanceName.IsEmpty() ? TEXT("Unnamed") : Rec.ActorInstanceName;
                        FString ClassName = Rec.SourceClass ? Rec.SourceClass->GetName() : TEXT("UnknownClass");
                        return FString::Printf(TEXT("Object '%s' (Class: %s, ItemId: %s) destroyed: %s"),
                            *Name, *ClassName, *ItemId.ToString(), bShouldBeDestroyed ? TEXT("true") : TEXT("false"));
                    }
                    */
                }
            }
        }
        return FString::Printf(TEXT("Object (ItemId: %s) destroyed: %s"), *ItemId.ToString(), bShouldBeDestroyed ? TEXT("true") : TEXT("false"));
    }

    AActor* Actor = ActorRef.Get();
    FString Name = IsValid(Actor) ? Actor->GetName() : TEXT("Unknown");
    return FString::Printf(TEXT("Object %s destroyed: %s"), *Name, bShouldBeDestroyed ? TEXT("true") : TEXT("false"));
}