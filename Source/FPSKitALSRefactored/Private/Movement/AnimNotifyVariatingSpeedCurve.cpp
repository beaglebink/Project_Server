// Fill out your copyright notice in the Description page of Project Settings.


#include "Movement/AnimNotifyVariatingSpeedCurve.h"

#include "Movement/CharacterMovementComponentVar.h"

#include "GameFramework/Character.h"
#include "Animation/AnimNotifyLibrary.h"

void UAnimNotifyVariatingSpeedCurve::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
}

void UAnimNotifyVariatingSpeedCurve::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	const AActor* ownerActor = IsValid(MeshComp) ? MeshComp->GetOwner() : nullptr;
	const ACharacter* ownerCharacter = IsValid(ownerActor) ? Cast<ACharacter>(ownerActor) : nullptr;
	UCharacterMovementComponent* characterMovementComponent = IsValid(ownerCharacter) ? ownerCharacter->GetCharacterMovement() : nullptr;
	UCharacterMovementComponentVar* characterMovementComponentVar = IsValid(characterMovementComponent) ? Cast<UCharacterMovementComponentVar>(characterMovementComponent) : nullptr;
	const FAnimNotifyEvent* notifyEvent = EventReference.GetNotify();
	if (IsValid(VariatingSpeedCurve) && notifyEvent != nullptr)
	{
		const float currentTime = UAnimNotifyLibrary::GetCurrentAnimationTime(EventReference);
		const float stateStartTime = notifyEvent->GetTriggerTime();
		const float stateDuration = notifyEvent->GetDuration();
		const float curveTime = (stateDuration > 0.f) ? FMath::Clamp((currentTime - stateStartTime) / stateDuration, 0.f, 1.f) : 0.f;
		const float speedState = VariatingSpeedCurve->GetFloatValue(curveTime);
		if (IsValid(characterMovementComponentVar))
		{
			characterMovementComponentVar->SetVariatingSpeedState(speedState);
		}
		if (IsValid(Animation) && IsValid(GEngine))
		{
			FString animationName = (IsValid(characterMovementComponentVar) ? "" : "                                                            ") + Animation->GetName() + " " + FString::SanitizeFloat(speedState);
			GEngine->AddOnScreenDebugMessage(-1, UE_KINDA_SMALL_NUMBER, FColor::Yellow, animationName);
		}
	}
}

void UAnimNotifyVariatingSpeedCurve::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	const AActor* ownerActor = IsValid(MeshComp) ? MeshComp->GetOwner() : nullptr;
	const ACharacter* ownerCharacter = IsValid(ownerActor) ? Cast<ACharacter>(ownerActor) : nullptr;
	UCharacterMovementComponent* characterMovementComponent = IsValid(ownerCharacter) ? ownerCharacter->GetCharacterMovement() : nullptr;
	UCharacterMovementComponentVar* characterMovementComponentVar = IsValid(characterMovementComponent) ? Cast<UCharacterMovementComponentVar>(characterMovementComponent) : nullptr;
	if (IsValid(characterMovementComponentVar))
	{
		characterMovementComponentVar->SetVariatingSpeedState(1.f);
	}

	Super::NotifyEnd(MeshComp, Animation, EventReference);
}