#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FloorPopulationTypes.h"
#include "GameplayTagContainer.h"
#include "FloorAssignmentComponent.generated.h"

class UInteriorSubsystem;

UENUM(BlueprintType)
enum class ESnapshotChannel : uint8
{
    None     UMETA(DisplayName = "None"),
    Snapshot UMETA(DisplayName = "Snapshot"),
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPSKITALSREFACTORED_API UFloorAssignmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFloorAssignmentComponent();

    FText InteriorSetName;
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "FloorAssignment")
    FGuid InteriorSetId;
    FText FloorName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "FloorAssignment")
    FGuid FloorId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "FloorAssignment")
    EFloorActorType ActorType = EFloorActorType::LightItem;
    FGuid AnchorId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "FloorAssignment")
	FGameplayTagContainer GameplayTagContainer;

    // DuplicateTransient — ID не копируется при дублировании
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, DuplicateTransient, SaveGame, Category = "FloorAssignment")
    FGuid ItemId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "FloorAssignment")
    FGuid ProtectedItemId;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, SaveGame, Category = "FloorAssignment")
    ESnapshotChannel SnapshotChannel = ESnapshotChannel::None;

    UFUNCTION(BlueprintCallable, Category = "FloorAssignment")
    FGuid GetInteriorSetId() const { return InteriorSetId; }

    UFUNCTION(BlueprintCallable, Category = "FloorAssignment")
    FGuid GetFloorId() const { return FloorId; }

    UFUNCTION(BlueprintCallable, Category = "FloorAssignment")
    void Registrate(EFloorActorType Type, const ESnapshotChannel& Channel = ESnapshotChannel::None, FGuid ForceItemId = FGuid());

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FloorAssignment")
    FGuid GetItemId() const { return ProtectedItemId; }

    void PublishRegistration();

    // Пометить уровень грязным (для сохранения)
    void MarkPackageDirty();

/** Заполняет InteriorSetId / FloorId / имена по данным текущего уровня.
 *  В редакторе — только из ассетов (AssetRegistry).
 *  В рантайме — из ALocationAnchorActor в текущем мире. */
    UFUNCTION(BlueprintCallable, Category = "FloorAssignment")
    void AutoFillFloorIdsFromWorld();

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssignment")
    void AutoFillFromCurrentLevel();

    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PostEditImport() override;   // Обработка дублирования и копирования
    virtual void OnRegister() override;
    virtual void PostInitProperties() override;
    virtual void PostLoad() override;

private:
    bool IsRuntimeSpawned = false;

    UInteriorSubsystem* GetInteriorSubsystem() const;
};