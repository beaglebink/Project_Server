// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/EnemyBaseAIController.h"

#include "Character/EnemyBasePathFollowingComponent.h"

AEnemyBaseAIController::AEnemyBaseAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UEnemyBasePathFollowingComponent>(TEXT("PathFollowingComponent")))
{
}