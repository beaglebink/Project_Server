#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FloorPopulationTypes.h"
#include "FloorAssignmentComponent.generated.h"

// Канал снапшота — расширяемый список, пока достаточен None / Snapshot
UENUM(BlueprintType)
enum class ESnapshotChannel : uint8
{
    None     UMETA(DisplayName = "None"),
    Snapshot UMETA(DisplayName = "Snapshot"),
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPSKITALSREFACTORED_API UFloorAssignmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFloorAssignmentComponent();

    // Имя интерьера (для визуализации)
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "FloorAssignment")
    FText InteriorSetName;

    // GUID интерьера (сохраняется в экземпляре уровня)
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "FloorAssignment")
    FGuid InteriorSetId;

    // Отображаемое имя этажа
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "FloorAssignment")
    FText FloorName;

    // GUID этажа (сохраняется в экземпляре уровня)
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "FloorAssignment")
    FGuid FloorId;

    // Тип актора на этаже
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorAssignment")
    EFloorActorType ActorType = EFloorActorType::LightItem;

    // Опциональный якорь (GUID)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorAssignment")
    FGuid AnchorId;

    // Стабильный идентификатор экземпляра для сопоставления с snapshot (сохраняется в .umap)
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, DuplicateTransient, Category = "FloorAssignment")
    FGuid ItemId;

    // Канал снапшота — если != None, актор участвует в сохранении/восстановлении
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "FloorAssignment")
    ESnapshotChannel SnapshotChannel = ESnapshotChannel::None;

    UFUNCTION(BlueprintCallable, Category = "FloorAssignment")
    FGuid GetInteriorSetId() const { return InteriorSetId; }

    UFUNCTION(BlueprintCallable, Category = "FloorAssignment")
    FGuid GetFloorId() const { return FloorId; }

    //virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override
    //{
    //    Super::PostDuplicate(DuplicateMode);

        //if (DuplicateMode == EDuplicateMode::Type::Normal) return;
        
        //ItemId = FGuid::NewGuid();
    //}

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
