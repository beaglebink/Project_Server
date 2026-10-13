#include "SceneActorRefFunctionLibrary.h"

#include "FloorAssignmentComponent.h"
#include "FloorAsset.h"
#include "WorldRegionAsset.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPath.h"

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

static UWorld* ResolveWorldFromSoftLevel(const TSoftObjectPtr<UWorld>& SoftLevel)
{
    if (SoftLevel.IsNull())
        return nullptr;

    if (UWorld* Loaded = SoftLevel.Get())
        return Loaded;

#if WITH_EDITOR
    const FSoftObjectPath Path = SoftLevel.ToSoftObjectPath();
    const FString PackageName = Path.GetLongPackageName();
    UPackage* Package = LoadPackage(nullptr, *PackageName, LOAD_None);
    if (Package)
    {
        return UWorld::FindWorldInPackage(Package);
    }
#endif

    return nullptr;
}

static TArray<AActor*> CollectActorsFromWorld(UWorld* World, bool bRequireFloorAssignment)
{
    TArray<AActor*> Result;
    if (!World)
        return Result;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor))
            continue;

        if (bRequireFloorAssignment)
        {
            UFloorAssignmentComponent* FAC =
                Actor->FindComponentByClass<UFloorAssignmentComponent>();
            if (!FAC || !FAC->ItemId.IsValid())
                continue;
        }

        Result.Add(Actor);
    }

    return Result;
}

static AActor* FindActorByLabelInWorld(UWorld* World, const FString& Label)
{
    if (!World || Label.IsEmpty())
        return nullptr;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor))
            continue;

        UFloorAssignmentComponent* FAC =
            Actor->FindComponentByClass<UFloorAssignmentComponent>();
        if (!FAC || !FAC->ItemId.IsValid())
            continue;

        if (Actor->GetActorLabel().Equals(Label, ESearchCase::CaseSensitive))
            return Actor;
    }

    return nullptr;
}

static AActor* FindActorByPersistentIdInWorld(UWorld* World, const FGuid& PersistentId)
{
    if (!World || !PersistentId.IsValid())
        return nullptr;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor))
            continue;

        UFloorAssignmentComponent* FAC =
            Actor->FindComponentByClass<UFloorAssignmentComponent>();
        if (FAC && FAC->ItemId == PersistentId)
            return Actor;
    }

    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Фабрика из AActor*
// ─────────────────────────────────────────────────────────────────────────────

FSceneActorRef USceneActorRefFunctionLibrary::MakeSceneActorRefFromActor(AActor* TargetActor)
{
    FSceneActorRef Ref;

    if (!IsValid(TargetActor))
        return Ref;

    if (UFloorAssignmentComponent* FAC =
        TargetActor->FindComponentByClass<UFloorAssignmentComponent>())
    {
        Ref.ActorPersistentId = FAC->ItemId;
    }

    Ref.ActorLabel = TargetActor->GetActorLabel();

    return Ref;
}

// ─────────────────────────────────────────────────────────────────────────────
// Готовые Ref'ы (find + make)
// ─────────────────────────────────────────────────────────────────────────────

FSceneActorRef USceneActorRefFunctionLibrary::FindSceneActorRefByLabelOnFloor(
    UFloorAsset* FloorAsset, const FString& ActorLabel)
{
    FSceneActorRef Result;
    if (AActor* Actor = FindActorByLabelOnFloor(FloorAsset, ActorLabel))
    {
        Result = MakeSceneActorRefFromActor(Actor);
    }
    return Result;
}

FSceneActorRef USceneActorRefFunctionLibrary::FindSceneActorRefByLabelOnRegion(
    UWorldRegionAsset* RegionAsset, const FString& ActorLabel)
{
    FSceneActorRef Result;
    if (AActor* Actor = FindActorByLabelOnRegion(RegionAsset, ActorLabel))
    {
        Result = MakeSceneActorRefFromActor(Actor);
    }
    return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Актёры по ассету сцены
// ─────────────────────────────────────────────────────────────────────────────

TArray<AActor*> USceneActorRefFunctionLibrary::GetActorsFromFloorAsset(
    UFloorAsset* FloorAsset, bool bRequireFloorAssignment)
{
    if (!FloorAsset)
        return {};

    UWorld* World = ResolveWorldFromSoftLevel(FloorAsset->FloorLevel);
    return CollectActorsFromWorld(World, bRequireFloorAssignment);
}

TArray<AActor*> USceneActorRefFunctionLibrary::GetActorsFromRegionAsset(
    UWorldRegionAsset* RegionAsset, bool bRequireFloorAssignment)
{
    if (!RegionAsset)
        return {};

    UWorld* World = ResolveWorldFromSoftLevel(RegionAsset->RegionLevel);
    return CollectActorsFromWorld(World, bRequireFloorAssignment);
}

AActor* USceneActorRefFunctionLibrary::FindActorByLabelOnFloor(
    UFloorAsset* FloorAsset, const FString& ActorLabel)
{
    if (!FloorAsset)
        return nullptr;

    UWorld* World = ResolveWorldFromSoftLevel(FloorAsset->FloorLevel);
    return FindActorByLabelInWorld(World, ActorLabel);
}

AActor* USceneActorRefFunctionLibrary::FindActorByLabelOnRegion(
    UWorldRegionAsset* RegionAsset, const FString& ActorLabel)
{
    if (!RegionAsset)
        return nullptr;

    UWorld* World = ResolveWorldFromSoftLevel(RegionAsset->RegionLevel);
    return FindActorByLabelInWorld(World, ActorLabel);
}

AActor* USceneActorRefFunctionLibrary::FindActorByPersistentId(const FGuid& PersistentId)
{
    if (!PersistentId.IsValid() || !GEngine)
        return nullptr;

    for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
    {
        UWorld* World = Ctx.World();
        if (!World || !World->IsGameWorld())
            continue;

        if (AActor* Found = FindActorByPersistentIdInWorld(World, PersistentId))
            return Found;
    }

    return nullptr;
}