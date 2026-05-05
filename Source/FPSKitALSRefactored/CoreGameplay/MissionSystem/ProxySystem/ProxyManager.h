#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProxyManager.generated.h"

class UActorProxy;
class UFloorAsset;

UCLASS()
class FPSKITALSREFACTORED_API UProxyManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Proxy")
    void RegisterProxy(UActorProxy* Proxy);

    UFUNCTION(BlueprintPure, Category = "Proxy")
    UActorProxy* GetActorByName(UFloorAsset* FloorAsset, FName ActorName) const;

    UFUNCTION(BlueprintPure, Category = "Proxy")
    TArray<UActorProxy*> GetActorsByClass(UFloorAsset* FloorAsset, TSubclassOf<AActor> ActorClass) const;

    UFUNCTION(BlueprintPure, Category = "Proxy")
    TArray<UActorProxy*> GetActorsByTag(UFloorAsset* FloorAsset, FName ActorTag) const;

#if WITH_EDITOR
    void ScanCurrentEditorScene(UFloorAsset* FloorAsset);
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    void OnMapOpened(const FString& Filename, bool bAsTemplate);
    TArray<UActorProxy*> GetActorsFromFloorAsset(UFloorAsset* FloorAsset, TSubclassOf<AActor> ActorClass = nullptr);
#endif

private:
    UPROPERTY() TArray<UActorProxy*> RegisteredProxies;
};
