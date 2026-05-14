#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "FloorPopulationTypes.h"
#include "FloorPlacementPayload.generated.h"

UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UFloorPlacementPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "FloorPlacement")
	FGuid InteriorSetId;

	UPROPERTY(BlueprintReadWrite, Category = "FloorPlacement")
	FGuid FloorId;

	UPROPERTY(BlueprintReadWrite, Category = "FloorPlacement")
	EFloorActorType ActorType = EFloorActorType::LightItem;

	UPROPERTY(BlueprintReadWrite, Category = "FloorPlacement")
	FGuid AnchorId;

    // Optional world transform of the placed actor
    // (Опциональная мировая трансформация размещённого актора)
    UPROPERTY(BlueprintReadWrite, Category = "FloorPlacement")
    FTransform WorldTransform = FTransform::Identity;

	// Optional stable id (from component) to identify the placed actor
	UPROPERTY(BlueprintReadWrite, Category = "FloorPlacement")
	FGuid ItemId;

	// Owner actor (weak ptr)
	//UPROPERTY(BlueprintReadWrite, Category = "FloorPlacement")
	//TWeakObjectPtr<AActor> OwnerActor;

	UFUNCTION(BlueprintCallable, Category = "FloorPlacement")
	UFloorPlacementPayload* Setup(
		const FGuid& InInteriorSetId,
		const FGuid& InFloorId,
		EFloorActorType InActorType,
		const FGuid& InAnchorId,
		const FTransform& InWorldTransform,
		const FGuid& InItemId/*,
		AActor* InOwner*/)
	{
		InteriorSetId = InInteriorSetId;
		FloorId = InFloorId;
		ActorType = InActorType;
		AnchorId = InAnchorId;
		WorldTransform = InWorldTransform;
		ItemId = InItemId;
		//OwnerActor = InOwner;
		return this;
	}
};