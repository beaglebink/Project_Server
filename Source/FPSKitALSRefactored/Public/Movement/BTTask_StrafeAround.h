// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BTTask_StrafeAround.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UBTTask_StrafeAround : public UBTTask_MoveTo
{
	GENERATED_BODY()

public:
	UBTTask_StrafeAround(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

protected:
	virtual EBTNodeResult::Type PerformMoveTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(Category = Strafe, EditAnywhere)
	FVector2D StrafeAngleRange = FVector2D(10.0, 15.0);
	UPROPERTY(Category = Strafe, EditAnywhere)
	FVector2D StrafeRadiusRange = FVector2D(400.0, 700.0);
	UPROPERTY(Category = Strafe, EditAnywhere)
	FVector2D StrafeDesiredPitchAngleRange = FVector2D(15.0, 45.0);

private:
	FVector GetStrafeTargetLocation(const UBehaviorTreeComponent& OwnerComp) const;
};