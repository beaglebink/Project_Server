// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/EnemyBasePathFollowingComponent.h"

#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

bool UEnemyBasePathFollowingComponent::HasReachedDestination(const FVector& CurrentLocation) const
{
	bool bResult = Super::HasReachedDestination(CurrentLocation);
	bResult |= HasReachedInternalFlight(CurrentLocation);
	return bResult;
}

bool UEnemyBasePathFollowingComponent::HasReachedCurrentTarget(const FVector& CurrentLocation) const
{
	bool bResult = Super::HasReachedCurrentTarget(CurrentLocation);
	return bResult;
}

bool UEnemyBasePathFollowingComponent::HasReachedInternalFlight(const FVector& CurrentLocation) const
{
	const AActor* controllerActor = GetOwner();
	const AAIController* controller = IsValid(controllerActor) ? Cast<AAIController>(controllerActor) : nullptr;
	const ACharacter* character = IsValid(controller) ? controller->GetCharacter() : nullptr;
	const UCharacterMovementComponent* characterMovementComponent = IsValid(character) ? character->GetCharacterMovement() : nullptr;
	if (Path.IsValid() && IsValid(characterMovementComponent) && characterMovementComponent->MovementMode == EMovementMode::MOVE_Flying)
	{
		const float acceptanceRangeToUse = GetFinalAcceptanceRadius(*Path, OriginalMoveRequestGoalLocation);
		const FVector currentTarget = GetCurrentTargetLocation();
		const double rangeToTarget = (currentTarget - CurrentLocation).Length();
		return (rangeToTarget < acceptanceRangeToUse);
	}
	return false;
}
