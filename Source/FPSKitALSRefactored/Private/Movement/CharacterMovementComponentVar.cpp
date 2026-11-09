// Fill out your copyright notice in the Description page of Project Settings.


#include "Movement/CharacterMovementComponentVar.h"

#include "CoreBlueprintFunctionLibrary.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

UCharacterMovementComponentVar::UCharacterMovementComponentVar(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	MaxFlySpeedVertical = MaxFlySpeed;
	MaxFlySpeedHorizontal = MaxFlySpeed;

	VerticalFlySkeletalMeshPitchScale = 1.f;
}

void UCharacterMovementComponentVar::PerformMovement(float DeltaTime)
{
	Super::PerformMovement(DeltaTime);

	ProcessAdditionalMeshRotaion(DeltaTime);
}

float UCharacterMovementComponentVar::GetMaxSpeed() const
{
	return Super::GetMaxSpeed() * VariatingSpeedState;
}

void UCharacterMovementComponentVar::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
}

void UCharacterMovementComponentVar::BeginPlay()
{
	Super::BeginPlay();

	const AActor* ownerActor = GetOwner();
	const ACharacter* ownerCharacter = IsValid(ownerActor) ? Cast<ACharacter>(ownerActor) : nullptr;
	const USkeletalMeshComponent* ownerSkeletalMesh = IsValid(ownerCharacter) ? ownerCharacter->GetMesh() : nullptr;
	MeshOriginalTransform = IsValid(ownerSkeletalMesh) ? ownerSkeletalMesh->GetRelativeTransform() : FTransform();
}

void UCharacterMovementComponentVar::SetVariatingSpeedState(float inVariatingSpeedState)
{
    VariatingSpeedState = FMath::Max(inVariatingSpeedState, 0.f);
}

void UCharacterMovementComponentVar::ApplyShift(FVector2D inShift)
{
	ACharacter* ownerCharacter = GetCharacterOwner();
	if (IsValid(ownerCharacter))
	{
		const FVector characterRightVector = ownerCharacter->GetActorRightVector();
		const FVector characterUpVector = ownerCharacter->GetActorUpVector();
		ownerCharacter->AddActorWorldOffset((characterRightVector * inShift.X + characterUpVector * inShift.Y), true);
	}
}

void UCharacterMovementComponentVar::SetAnimationAddedPitch(float inAnimationAddedPitch)
{
	AnimationAddedPitch = inAnimationAddedPitch;
}

void UCharacterMovementComponentVar::SetVerticalFlySkeletalMeshPitchCoupling(float inPitchCoupling)
{
	VerticalFlySkeletalMeshPitchScale = inPitchCoupling;
}

void UCharacterMovementComponentVar::ProcessAdditionalMeshRotaion(float DeltaTime)
{
	const AActor* ownerActor = GetOwner();
	const ACharacter* ownerCharacter = IsValid(ownerActor) ? Cast<ACharacter>(ownerActor) : nullptr;
	USkeletalMeshComponent* ownerSkeletalMesh = IsValid(ownerCharacter) ? ownerCharacter->GetMesh() : nullptr;
	if (IsValid(ownerSkeletalMesh))
	{
		const double horizontalSpeed = Velocity.Size2D();
		const float climbAngle = FMath::RadiansToDegrees(FMath::Atan2(Velocity.Z, horizontalSpeed));
		const FRotator additionalMeshRotationDesired = FRotator(climbAngle * VerticalFlySkeletalMeshPitchScale + AnimationAddedPitch * 90.0, 0.0, 0.0);
		const FRotator deltaRotationDesired = additionalMeshRotationDesired - AdditionalMeshRotaton;
		const FRotator deltaRotationMax = GetDeltaRotation(DeltaTime);
		const FRotator deltaRotation = FRotator(FMath::Clamp(deltaRotationDesired.Pitch, -deltaRotationMax.Pitch, deltaRotationMax.Pitch), FMath::Clamp(deltaRotationDesired.Yaw, -deltaRotationMax.Yaw, deltaRotationMax.Yaw), FMath::Clamp(deltaRotationDesired.Roll, -deltaRotationMax.Roll, deltaRotationMax.Roll));
		const FRotator nextRotation = AdditionalMeshRotaton + deltaRotation;
		const FQuat nextRotationQuat = nextRotation.Quaternion();
		const FTransform nextTransform = UCoreBlueprintFunctionLibrary::RotateTransformAroundPoint(MeshOriginalTransform, FVector::ZeroVector, nextRotationQuat);

		ownerSkeletalMesh->SetRelativeTransform(nextTransform);

		AdditionalMeshRotaton = nextRotation;

#if WITH_EDITOR
		if (IsValid(GEngine))
		{
			FString climbInfo = ownerCharacter->GetName() + " climb angle: " + FString::SanitizeFloat(climbAngle) + " pitch scale: " + FString::SanitizeFloat(VerticalFlySkeletalMeshPitchScale);
			GEngine->AddOnScreenDebugMessage(-1, UE_KINDA_SMALL_NUMBER, FColor::Yellow, climbInfo);
		}
#endif
	}
}
