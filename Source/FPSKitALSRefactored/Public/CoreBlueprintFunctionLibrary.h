// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputTriggers.h"
#include "InputModifiers.h"
#include "CoreBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class FPSKITALSREFACTORED_API UCoreBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Input")
	static void SimulateKeyPress(APlayerController* PlayerController, UInputAction* InputAction);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General")
	static bool IsPIE();
};
