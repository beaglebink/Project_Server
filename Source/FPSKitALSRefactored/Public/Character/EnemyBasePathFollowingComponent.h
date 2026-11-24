// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Navigation/PathFollowingComponent.h"
#include "EnemyBasePathFollowingComponent.generated.h"

/**
 * 
 */
UCLASS()
class FPSKITALSREFACTORED_API UEnemyBasePathFollowingComponent : public UPathFollowingComponent
{
	GENERATED_BODY()
	
protected:
	virtual bool HasReachedDestination(const FVector& CurrentLocation) const override;
	virtual bool HasReachedCurrentTarget(const FVector& CurrentLocation) const override;
	bool HasReachedInternalFlight(const FVector& CurrentLocation) const;
};
