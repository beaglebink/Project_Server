#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../SpawnGroupSystem/SpawnGroupAsset.h"
#include "../SpawnGroupSystem/GhostClearedPayload.h"
#include "../SpawnGroupSystem/SpawnVolume.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "SpawnGroupSpawner.generated.h"

class UFloorAssignmentComponent;

UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API ASpawnGroupSpawner : public AActor
{
    GENERATED_BODY()

public:
    ASpawnGroupSpawner();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // ===== Конфигурация =====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    TObjectPtr<USpawnGroupAsset> SpawnGroupAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    bool bSpawnOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    bool bDestroyOnClear = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    bool bAllowRespawn = false;

    /** Список локаций спавна (объёмы ASpawnVolume или простые точки-акторы) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Placement")
    TArray<TObjectPtr<ASpawnVolume>> SpawnLocations;

    /** Автоматически собирать дочерние акторы, помеченные тегом "SpawnPoint" или наследников ASpawnVolume */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Placement")
    bool bAutoGatherSpawnLocations = true;

    /** Глобальное смещение позиции спавна относительно выбранной локации */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Placement")
    FVector SpawnOffset = FVector::ZeroVector;

    /** Поворот по умолчанию, если локация не задаёт свой */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Placement")
    FRotator DefaultSpawnRotation = FRotator::ZeroRotator;

#if WITH_EDITOR
    virtual void Tick(float DeltaTime) override;
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
    void DrawEditorLines() const;
#endif

    // ===== Состояние (доступно через геттеры) =====
private:
    UPROPERTY(VisibleAnywhere, Category = "SpawnGroup|State")
    TArray<TObjectPtr<AActor>> SpawnedGhosts;  // Сильные ссылки, но следим за уничтожением

    UPROPERTY(VisibleAnywhere, Category = "SpawnGroup|State")
    FSpawnGroupId RuntimeGroupId;

    UPROPERTY(VisibleAnywhere, Category = "SpawnGroup|State")
    ESpawnGroupStatus CurrentStatus = ESpawnGroupStatus::Inactive;

    TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;
    bool bHasPublishedClear = false;

public:
    // Геттеры для Blueprint
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpawnGroup|State")
    TArray<AActor*> GetSpawnedGhosts() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpawnGroup|State")
    FSpawnGroupId GetRuntimeGroupId() const { return RuntimeGroupId; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpawnGroup|State")
    ESpawnGroupStatus GetCurrentStatus() const { return CurrentStatus; }

    // ===== Методы =====
    UFUNCTION(BlueprintCallable, Category = "SpawnGroup")
    void SpawnGroup();

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup")
    void ClearGroup(ESpawnGroupResolutionReason Reason = ESpawnGroupResolutionReason::Eliminated);

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup")
    void ResetGroup();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpawnGroup")
    int32 GetAliveGhostCount() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpawnGroup")
    bool IsGroupCleared() const;

protected:
    ASpawnVolume* GetRandomSpawnLocation() const;
    FTransform GetTransformFromLocation(ASpawnVolume* Location) const;
    AActor* SpawnSingleGhost(TSubclassOf<AActor> ActorClass, const FTransform& Transform);
    void PublishGhostClearedEvent(ESpawnGroupResolutionReason Reason);
    void UpdateGroupStatus();
    void GatherSpawnLocationsFromChildren();

    UFUNCTION()
    void OnGhostDestroyed(AActor* DestroyedActor);
};