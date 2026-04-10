#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FloorPopulationTypes.h"
#include "FloorAssignmentComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPSKITALSREFACTORED_API UFloorAssignmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFloorAssignmentComponent();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FloorAssignment")
	FText InteriorSetName;

	UPROPERTY(VisibleAnywhere, Category = "FloorAssignment")
	FGuid InteriorSetId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FloorAssignment")
	FText FloorName;

	UPROPERTY(VisibleAnywhere, Category = "FloorAssignment")
	FGuid FloorId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorAssignment")
	EFloorActorType ActorType = EFloorActorType::LightItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorAssignment")
	FGuid AnchorId;

	FGuid ItemId;

	UFUNCTION(BlueprintCallable, Category = "FloorAssignment")
	FGuid GetInteriorSetId() const { return InteriorSetId; }

	UFUNCTION(BlueprintCallable, Category = "FloorAssignment")
	FGuid GetFloorId() const { return FloorId; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};