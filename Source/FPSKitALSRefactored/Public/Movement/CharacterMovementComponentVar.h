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

protected:
	virtual void PerformMovement(float DeltaTime) override;
	
public:
	UPROPERTY(Category = "Character Movement: Flying", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MaxFlySpeedVertical;

	UPROPERTY(Category = "Character Movement: Flying", EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MaxFlySpeedHorizontal;

	//UPROPERTY(Category = "Character Movement: Flying", EditAnywhere, BlueprintReadWrite)
	float VerticalFlySkeletalMeshPitchScale = 1.f;

	virtual float GetMaxSpeed() const override;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
	virtual void BeginPlay() override;

	UFUNCTION(Category = "Character Movement", BlueprintCallable)
	void SetVariatingSpeedState(float inVariatingSpeedState);

	UFUNCTION(Category = "Character Movement", BlueprintCallable)
	void ApplyShift(FVector2D inShift);

	UFUNCTION(Category = "Character Movement", BlueprintCallable)
	void SetAnimationAddedPitch(float inAnimationAddedPitch);

	UFUNCTION(Category = "Character Movement", BlueprintCallable)
	void SetVerticalFlySkeletalMeshPitchCoupling(float inPitchCoupling);

private:
	FTransform MeshOriginalTransform = FTransform();
	FRotator AdditionalMeshRotaton = FRotator();
	float VariatingSpeedState = 1.f;
	float AnimationAddedPitch = 0.f;

	void ProcessAdditionalMeshRotaion(float DeltaTime);
};
