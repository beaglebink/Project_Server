#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FloorPopulationTypes.h"
#include "FloorAssignmentComponent.generated.h"

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
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, SaveGame, Category = "FloorAssignment")
    FGuid FloorId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "FloorAssignment")
    EFloorActorType ActorType = EFloorActorType::LightItem;
    FGuid AnchorId;

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
    void Registrate(EFloorActorType Type, FGuid ForceItemId = FGuid())
    {
        ItemId = ForceItemId.IsValid() ? ForceItemId : FGuid::NewGuid();
        ProtectedItemId = ItemId;
        SnapshotChannel = ESnapshotChannel::Snapshot;
        ActorType = Type;
        MarkPackageDirty();
    }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FloorAssignment")
    FGuid GetItemId() const { return ProtectedItemId; }

    // Пометить уровень грязным (для сохранения)
    void MarkPackageDirty();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PostEditImport() override;   // Обработка дублирования и копирования
};