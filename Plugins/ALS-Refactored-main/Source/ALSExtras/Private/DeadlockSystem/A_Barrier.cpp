// Fill out your copyright notice in the Description page of Project Settings.


#include "DeadlockSystem/A_Barrier.h"

// Sets default values
AA_Barrier::AA_Barrier()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AA_Barrier::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AA_Barrier::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

