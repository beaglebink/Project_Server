#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ChoreFloorActorInterface.generated.h"

class UChoreDefinition;

UINTERFACE(BlueprintType)
class FPSKITALSREFACTORED_API UChoreFloorActorInterface : public UInterface
{
	GENERATED_BODY()
};

/** Implemented by actors that represent a chore in a loaded floor. */
class FPSKITALSREFACTORED_API IChoreFloorActorInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Chore|Floor")
	UChoreDefinition* GetChoreDefinition() const;
	virtual UChoreDefinition* GetChoreDefinition_Implementation() const { return nullptr; }

	/** Called when a mission takes or releases this actor's floor. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Chore|Floor")
	void SetMissionFloorBlocked(bool bBlocked);
	virtual void SetMissionFloorBlocked_Implementation(bool bBlocked) {}
};
