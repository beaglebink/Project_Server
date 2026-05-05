#include "ProxyManager.h"
#include "ActorProxy.h"
#include "FloorAsset.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

void UProxyManager::RegisterProxy(UActorProxy* Proxy)
{
    if (Proxy && !RegisteredProxies.Contains(Proxy))
        RegisteredProxies.Add(Proxy);
}

UActorProxy* UProxyManager::GetActorByName(UFloorAsset* FloorAsset, FName ActorName) const
{
    for (UActorProxy* Proxy : RegisteredProxies)
        if (Proxy && Proxy->GetFloorAsset() == FloorAsset && Proxy->GetActorName() == ActorName)
            return Proxy;
    return nullptr;
}

TArray<UActorProxy*> UProxyManager::GetActorsByTag(UFloorAsset* FloorAsset, FName ActorTag) const
{
    TArray<UActorProxy*> Result;
    for (UActorProxy* Proxy : RegisteredProxies)
        if (Proxy && Proxy->GetFloorAsset() == FloorAsset && Proxy->HasTag(ActorTag))
            Result.Add(Proxy);
    return Result;
}

TArray<UActorProxy*> UProxyManager::GetActorsByClass(UFloorAsset* FloorAsset, TSubclassOf<AActor> ActorClass) const
{
    TArray<UActorProxy*> Result;
    for (UActorProxy* Proxy : RegisteredProxies)
        if (Proxy && Proxy->GetFloorAsset() == FloorAsset && Proxy->IsOfClass(ActorClass))
            Result.Add(Proxy);
    return Result;
}

#if WITH_EDITOR
void UProxyManager::ScanCurrentEditorScene(UFloorAsset* FloorAsset)
{
    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
    if (!EditorWorld || !FloorAsset) return;

    RegisteredProxies.Empty();

    for (AActor* Actor : EditorWorld->GetCurrentLevel()->Actors)
    {
        if (!Actor) continue;
        UActorProxy* Proxy = NewObject<UActorProxy>(this);
        Proxy->FloorAsset = FloorAsset;
        Proxy->ActorName = Actor->GetFName();
        Proxy->ActorClass = Actor->GetClass();
        Proxy->Tags = Actor->Tags;
        RegisterProxy(Proxy);
    }
}

void UProxyManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    FEditorDelegates::OnMapOpened.AddUObject(this, &UProxyManager::OnMapOpened);
}

void UProxyManager::OnMapOpened(const FString& Filename, bool bAsTemplate)
{
    // можно пересканировать сцену
}

TArray<UActorProxy*> UProxyManager::GetActorsFromFloorAsset(UFloorAsset* FloorAsset, TSubclassOf<AActor> ActorClass)
{
    TArray<UActorProxy*> Results;
    if (!FloorAsset) return Results;

    // Получаем путь к Package уровня
    FString PackageName = FloorAsset->FloorLevel.GetLongPackageName();
    if (PackageName.IsEmpty()) return Results;

    // Загружаем Package (НЕ загружает уровень в мир)
    UPackage* Package = LoadPackage(nullptr, *PackageName, LOAD_None);
    if (!Package) return Results;

    // Ищем UWorld внутри Package
    UWorld* World = nullptr;
    TArray<UObject*> Objects;
    GetObjectsWithOuter(Package, Objects, true);
    for (UObject* Obj : Objects)
    {
        if (UWorld* W = Cast<UWorld>(Obj))
        {
            World = W;
            break;
        }
    }

    // Читаем акторы из PersistentLevel
    if (World && World->PersistentLevel)
    {
        for (AActor* Actor : World->PersistentLevel->Actors)
        {
            if (!Actor) continue;
            if (ActorClass && !Actor->IsA(ActorClass)) continue;

            UActorProxy* Proxy = NewObject<UActorProxy>(this);
            Proxy->FloorAsset = FloorAsset;
            Proxy->ActorName = Actor->GetFName();
            Proxy->ActorClass = Actor->GetClass();
            Proxy->Tags = Actor->Tags;
            Results.Add(Proxy);
        }
    }

    return Results;
}
#endif
