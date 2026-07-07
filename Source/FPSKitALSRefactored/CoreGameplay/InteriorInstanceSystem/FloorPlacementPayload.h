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

	UPROPERTY(BlueprintReadWrite, Category = "FloorPlacement")
	UClass* ActorClass;

	// Owner actor (weak ptr)
	UPROPERTY(BlueprintReadWrite, Category = "FloorPlacement")
	TWeakObjectPtr<AActor> Actor;

	UPROPERTY(BlueprintReadWrite, Category = "FloorPlacement")
	TArray<FGameplayTag> GameplayTags;

	UPROPERTY(BlueprintReadWrite, Category = "FloorPlacement")
	TArray<FName> TextTags;

	UFUNCTION(BlueprintCallable, Category = "FloorPlacement")
	UFloorPlacementPayload* Setup(
		EFloorActorType InActorType,
			const FTransform& InWorldTransform,
		const FGuid& InItemId,
		UClass* Class = nullptr,
		UClass* InActorClass = nullptr)
	{
		ActorType = InActorType;
		WorldTransform = InWorldTransform;
		ItemId = InItemId;
		ActorClass = Class;

		return this;
	}

	UFUNCTION(BlueprintCallable, Category = "FloorPlacement")
	UFloorPlacementPayload* SetupWithTags(
		EFloorActorType InActorType,
		const FTransform& InWorldTransform,
		const FGuid& InItemId,
		UClass* Class,
		const TArray<FGameplayTag>& InGameplayTags,
		const TArray<FName>& InTextTags)
	{
		ActorType = InActorType;
		WorldTransform = InWorldTransform;
		ItemId = InItemId;
		ActorClass = Class;
		GameplayTags = InGameplayTags;
		TextTags = InTextTags;
		return this;
	}
};