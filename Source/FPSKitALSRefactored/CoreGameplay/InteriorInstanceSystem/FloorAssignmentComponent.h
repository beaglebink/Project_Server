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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorAssignment")
	FGuid InteriorSetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorAssignment")
	FGuid FloorId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorAssignment")
	EFloorActorType ActorType = EFloorActorType::LightItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FloorAssignment")
	FGuid AnchorId;

	// Stable ItemId — генерируется автоматически; скрыт от Blueprint и от деталей (дизайнеры GUID обычно не используют)
	UPROPERTY(EditAnywhere, Category = "FloorAssignment", meta = (HideInInspector))
	FGuid ItemId;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};