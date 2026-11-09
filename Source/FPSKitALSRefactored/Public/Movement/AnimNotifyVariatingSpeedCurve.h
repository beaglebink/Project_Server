// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyVariatingSpeedCurve.generated.h"

/**
 * 
 */
UCLASS()
class FPSKITALSREFACTORED_API UAnimNotifyVariatingSpeedCurve : public UAnimNotifyState
{
	GENERATED_BODY()

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCurveFloat> VariatingSpeedCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCurveFloat> LateralShiftCurve = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCurveFloat> VerticalShiftCurve = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UCurveFloat> PitchShiftCurve = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VerticalFlySkeletalMeshPitchCoupling = 1.f;

#if WITH_EDITOR
private:
	TMap<ACharacter*, FVector> PreviousLocations;
#endif
};
