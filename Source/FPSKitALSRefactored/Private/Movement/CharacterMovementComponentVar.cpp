// Fill out your copyright notice in the Description page of Project Settings.


#include "Movement/CharacterMovementComponentVar.h"

UCharacterMovementComponentVar::UCharacterMovementComponentVar(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	MaxWalkSpeedMin = MaxWalkSpeed;
	MaxFlySpeedVertical = MaxFlySpeed;
	MaxFlySpeedHorizontal = MaxFlySpeed;
}

void UCharacterMovementComponentVar::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);

    FVector tempVelocity = Velocity;
    const float speed = Velocity.Length();
    Velocity = Velocity.GetSafeNormal() * FMath::Min((MovementMode == MOVE_Flying ? MaxFlySpeed : MaxWalkSpeed) * VariatingSpeedState, speed);
}

void UCharacterMovementComponentVar::SetVariatingSpeedState(float inVariatingSpeedState)
{
    VariatingSpeedState = FMath::Max(inVariatingSpeedState, 0.f);
}
