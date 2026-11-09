// Fill out your copyright notice in the Description page of Project Settings.


#include "Movement/AnimNotifyVariatingSpeedCurve.h"

#include "CoreBlueprintFunctionLibrary.h"
#include "Movement/CharacterMovementComponentVar.h"

#include "Animation/AnimNotifyLibrary.h"
#include "Animation/SkeletalMeshActor.h"
#include "GameFramework/Character.h"

void UAnimNotifyVariatingSpeedCurve::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* ownerActor = IsValid(MeshComp) ? MeshComp->GetOwner() : nullptr;
	ACharacter* ownerCharacter = IsValid(ownerActor) ? Cast<ACharacter>(ownerActor) : nullptr;
	UCharacterMovementComponent* characterMovementComponent = IsValid(ownerCharacter) ? ownerCharacter->GetCharacterMovement() : nullptr;
	UCharacterMovementComponentVar* characterMovementComponentVar = IsValid(characterMovementComponent) ? Cast<UCharacterMovementComponentVar>(characterMovementComponent) : nullptr;
	if (IsValid(characterMovementComponentVar))
	{
		characterMovementComponentVar->SetVariatingSpeedState(1.f);
		characterMovementComponentVar->SetAnimationAddedPitch(0.f);
		characterMovementComponentVar->SetVerticalFlySkeletalMeshPitchCoupling(VerticalFlySkeletalMeshPitchCoupling);
	}
}

void UAnimNotifyVariatingSpeedCurve::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	AActor* ownerActor = IsValid(MeshComp) ? MeshComp->GetOwner() : nullptr;
	ACharacter* ownerCharacter = IsValid(ownerActor) ? Cast<ACharacter>(ownerActor) : nullptr;
	UCharacterMovementComponent* characterMovementComponent = IsValid(ownerCharacter) ? ownerCharacter->GetCharacterMovement() : nullptr;
	UCharacterMovementComponentVar* characterMovementComponentVar = IsValid(characterMovementComponent) ? Cast<UCharacterMovementComponentVar>(characterMovementComponent) : nullptr;
	const FAnimNotifyEvent* notifyEvent = EventReference.GetNotify();
	if (notifyEvent != nullptr)
	{
		const float currentTime = UAnimNotifyLibrary::GetCurrentAnimationTime(EventReference);
		const float stateStartTime = notifyEvent->GetTriggerTime();
		const float stateDuration = notifyEvent->GetDuration();
		const float curveTime = (stateDuration > 0.f) ? FMath::Clamp((currentTime - stateStartTime) / stateDuration, 0.f, 1.f) : 0.f;
		const float speedState = IsValid(VariatingSpeedCurve) ? VariatingSpeedCurve->GetFloatValue(curveTime) : 1.f;
		const float lateralShift = IsValid(LateralShiftCurve) ? LateralShiftCurve->GetFloatValue(curveTime) : 0.f;
		const float verticalShift = IsValid(VerticalShiftCurve) ? VerticalShiftCurve->GetFloatValue(curveTime) : 0.f;
		const float pitchShift = IsValid(PitchShiftCurve) ? PitchShiftCurve->GetFloatValue(curveTime) : 0.f;
		if (IsValid(ownerCharacter) && IsValid(characterMovementComponentVar))
		{
			characterMovementComponentVar->SetVariatingSpeedState(speedState);
			characterMovementComponentVar->ApplyShift(FVector2D(lateralShift, verticalShift));
			characterMovementComponentVar->SetAnimationAddedPitch(pitchShift);
		}
#if WITH_EDITOR
		if (IsValid(GEngine))
		{
			if (IsValid(Animation))
			{
				const float animationLength = Animation->GetPlayLength();
				const FString animationName = Animation->GetName();
				const float animationCurrentFrame = Animation->GetFrameAtTime(currentTime);
				const float currentSpeed = IsValid(characterMovementComponentVar) ? characterMovementComponentVar->Velocity.Length() : -1.f;
				const FString animationInfo = (IsValid(characterMovementComponentVar) ? "" : "                                                            ") +
					animationName + " speed multiplier: " + FString::SanitizeFloat(speedState, 3) +
					" pitch: " + FString::SanitizeFloat(pitchShift, 3) +
					(currentSpeed >= UE_KINDA_SMALL_NUMBER ? " speed: " + FString::SanitizeFloat(currentSpeed, 3) : "") +
					" frame: " + FString::SanitizeFloat(animationCurrentFrame, 3);
				GEngine->AddOnScreenDebugMessage(-1, UE_KINDA_SMALL_NUMBER, FColor::Yellow, animationInfo);
			}
			if (currentTime <= FrameDeltaTime && IsValid(ownerCharacter))
			{
				const FVector characterLocation = ownerCharacter->GetActorLocation();
				FVector& characterPreviousLocation = PreviousLocations.FindOrAdd(ownerCharacter);
				const FString cycleShift = (characterPreviousLocation.Equals(FVector::ZeroVector)) ? "" : FString::SanitizeFloat((characterLocation - characterPreviousLocation).Length());
				const FString cycleInfo = ownerCharacter->GetName() + " location: " + characterLocation.ToString() + " cycle shift: " + cycleShift;
				GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Yellow, cycleInfo);
				characterPreviousLocation = characterLocation;
			}
		}
#endif
	}
}

void UAnimNotifyVariatingSpeedCurve::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* ownerActor = IsValid(MeshComp) ? MeshComp->GetOwner() : nullptr;
	ACharacter* ownerCharacter = IsValid(ownerActor) ? Cast<ACharacter>(ownerActor) : nullptr;
	UCharacterMovementComponent* characterMovementComponent = IsValid(ownerCharacter) ? ownerCharacter->GetCharacterMovement() : nullptr;
	UCharacterMovementComponentVar* characterMovementComponentVar = IsValid(characterMovementComponent) ? Cast<UCharacterMovementComponentVar>(characterMovementComponent) : nullptr;
	if (IsValid(characterMovementComponentVar))
	{
		characterMovementComponentVar->SetVariatingSpeedState(1.f);
		characterMovementComponentVar->SetAnimationAddedPitch(0.f);
		characterMovementComponentVar->SetVerticalFlySkeletalMeshPitchCoupling(1.f);
	}

	Super::NotifyEnd(MeshComp, Animation, EventReference);
}
