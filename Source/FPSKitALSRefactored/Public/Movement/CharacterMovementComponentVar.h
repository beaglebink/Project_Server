// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CharacterMovementComponentVar.generated.h"

/**
 * 
 */
UCLASS()
class FPSKITALSREFACTORED_API UCharacterMovementComponentVar : public UCharacterMovementComponent
{
	GENERATED_BODY()

	UCharacterMovementComponentVar(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
public:
	UPROPERTY(Category = "Character Movement: Walking", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MaxWalkSpeedMin;

	UPROPERTY(Category = "Character Movement: Flying", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MaxFlySpeedVertical;

	UPROPERTY(Category = "Character Movement: Flying", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MaxFlySpeedHorizontal;

	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

	UPROPERTY(Category = "Character Movement", EditAnywhere, BlueprintReadWrite)
	float VariatingSpeedState = 1.f;

	UFUNCTION(Category = "Character Movement", BlueprintCallable)
	void SetVariatingSpeedState(float inVariatingSpeedState);
};
