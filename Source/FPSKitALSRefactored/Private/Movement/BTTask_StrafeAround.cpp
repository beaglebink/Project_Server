// Fill out your copyright notice in the Description page of Project Settings.


#include "Movement/BTTask_StrafeAround.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tasks/AITask_MoveTo.h"

UBTTask_StrafeAround::UBTTask_StrafeAround(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("StrafeAround");

	bUsePathfinding = false;
	bProjectGoalLocation = true;
	bTrackMovingGoal = false;
	bRequireNavigableEndLocation = false;
	bProjectGoalLocation = false;
}

void UBTTask_StrafeAround::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	AAIController* controller = IsValid(&OwnerComp) ? OwnerComp.GetAIOwner() : nullptr;
	if (IsValid(controller))
		controller->ClearFocus(EAIFocusPriority::Gameplay);

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

EBTNodeResult::Type UBTTask_StrafeAround::PerformMoveTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type result = EBTNodeResult::Failed;

	const UBlackboardComponent* blackboard = IsValid(&OwnerComp) ? OwnerComp.GetBlackboardComponent() : nullptr;
	FBTMoveToTaskMemory* memory = (NodeMemory != nullptr) ? CastInstanceNodeMemory<FBTMoveToTaskMemory>(NodeMemory) : nullptr;
	AAIController* controller = IsValid(&OwnerComp) ? OwnerComp.GetAIOwner() : nullptr;
	if (IsValid(controller) && IsValid(blackboard) && memory != nullptr)
	{
		FAIMoveRequest moveRequest;
		moveRequest.SetNavigationFilter(IsValid(FilterClass) ? FilterClass : controller->GetDefaultNavigationFilterClass());
		moveRequest.SetAllowPartialPath(bAllowPartialPath);
		moveRequest.SetAcceptanceRadius(AcceptableRadius);
		moveRequest.SetCanStrafe(bAllowStrafe);
		moveRequest.SetReachTestIncludesAgentRadius(bReachTestIncludesAgentRadius);
		moveRequest.SetReachTestIncludesGoalRadius(bReachTestIncludesGoalRadius);
		moveRequest.SetRequireNavigableEndLocation(bRequireNavigableEndLocation);
		moveRequest.SetProjectGoalLocation(bProjectGoalLocation);
		moveRequest.SetUsePathfinding(bUsePathfinding);

		const FVector targetLocation = GetStrafeTargetLocation(OwnerComp);

		const ACharacter* character = controller->GetCharacter();
		const FVector location = character->GetActorLocation();
		const FVector directionFromTarget = location - targetLocation;
		const double distanceFromTarget = directionFromTarget.Length();
		const FRotator rotationFromTarget = directionFromTarget.Rotation();
		const FRotator rotationFromTargetDesired = FRotator(FMath::RandRange(StrafeDesiredPitchAngleRange.X, StrafeDesiredPitchAngleRange.Y), FMath::RandRange(-180.0, 180.0), 0.0);
		const FRotator rotationFromTargetDesiredDelta = rotationFromTargetDesired - rotationFromTarget;
		const double strafeAngle = FMath::RandRange(StrafeAngleRange.X, StrafeAngleRange.Y);
		const FRotator rotationFromTargetDelta = FRotator(FMath::Clamp(rotationFromTargetDesiredDelta.Pitch, -strafeAngle, strafeAngle), FMath::Clamp(rotationFromTargetDesiredDelta.Yaw, -strafeAngle, strafeAngle), 0.0);
		const FRotator rotationFromTargetNext = rotationFromTarget + rotationFromTargetDelta;
		const double distanceFromTargetNext = FMath::RandRange(StrafeRadiusRange.X, StrafeRadiusRange.Y);
		const FVector directionFromTargetNext = rotationFromTargetNext.Vector() * distanceFromTargetNext;
		const FVector nextLocation = directionFromTargetNext + targetLocation;

		moveRequest.SetGoalLocation(nextLocation);

		if (moveRequest.IsValid() && (distanceFromTarget <= StrafeRadiusRange.Y))
		{
			UAITask_MoveTo* moveTask = memory->Task.Get();
			const bool bReuseExistingTask = (moveTask != nullptr);

			moveTask = PrepareMoveTask(OwnerComp, moveTask, moveRequest);
			if (IsValid(moveTask))
			{
				memory->bObserverCanFinishTask = false;

				if (bReuseExistingTask)
				{
					if (moveTask->IsActive())
					{
						UE_VLOG(controller, LogBehaviorTree, Verbose, TEXT("\'%s\' reusing AITask %s"), *GetNodeName(), *moveTask->GetName());
						moveTask->ConditionalPerformMove();
					}
					else
					{
						UE_VLOG(controller, LogBehaviorTree, Verbose, TEXT("\'%s\' reusing AITask %s, but task is not active - handing over move performing to task mechanics"), *GetNodeName(), *moveTask->GetName());
					}
				}
				else
				{
					memory->Task = moveTask;
					UE_VLOG(controller, LogBehaviorTree, Verbose, TEXT("\'%s\' task implementing move with task %s"), *GetNodeName(), *moveTask->GetName());
					moveTask->ReadyForActivation();
				}

				memory->bObserverCanFinishTask = true;
				result = (moveTask->GetState() != EGameplayTaskState::Finished) ? EBTNodeResult::InProgress :
					moveTask->WasMoveSuccessful() ? EBTNodeResult::Succeeded :
					EBTNodeResult::Failed;

				if (result != EBTNodeResult::Failed)
					controller->SetFocalPoint(targetLocation);
			}
		}
	}

	return result;
}

FVector UBTTask_StrafeAround::GetStrafeTargetLocation(const UBehaviorTreeComponent& OwnerComp) const
{
	FVector targetLocation = FVector::ZeroVector;

	const UBlackboardComponent* blackboard = IsValid(&OwnerComp) ? OwnerComp.GetBlackboardComponent() : nullptr;
	AAIController* controller = IsValid(&OwnerComp) ? OwnerComp.GetAIOwner() : nullptr;
	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
	{
		UObject* blackboardKeyValue = blackboard->GetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID());
		AActor* targetActor = Cast<AActor>(blackboardKeyValue);
		if (IsValid(targetActor))
		{
			targetLocation = targetActor->GetActorLocation();
		}
		else
		{
			UE_VLOG(controller, LogBehaviorTree, Warning, TEXT("UBTTask_StrafeAround::GetStrafeTargetLocation BB %s entry was empty"), *BlackboardKey.SelectedKeyName.ToString());
		}
	}
	else if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		const FVector TargetLocation = blackboard->GetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID());
		targetLocation = blackboard->GetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID());
	}

	return targetLocation;
}
