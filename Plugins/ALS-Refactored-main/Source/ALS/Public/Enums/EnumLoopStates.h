#pragma once

#include "EnhancedInputComponent.h"
#include "CoreMinimal.h"
#include "EnumLoopStates.generated.h"

UENUM(BlueprintType)
enum class EnumLoopStates :uint8
{
	None		UMETA(DisplayName = "No Action"),
	Sprint		UMETA(DisplayName = "On Sprint"),
	Walk		UMETA(DisplayName = "On Walk"),
	Crouch		UMETA(DisplayName = "On Crouch"),
	Jump		UMETA(DisplayName = "On Jump"),
	Aim			UMETA(DisplayName = "On Aim"),
	Ragdoll		UMETA(DisplayName = "On Ragdoll"),
	Roll		UMETA(DisplayName = "On Roll")
};

USTRUCT(BlueprintType)
struct FLoopEffectFrame
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "LoopEffect")
	EnumLoopStates FrameState;

	UPROPERTY(BlueprintReadWrite, Category = "LoopEffect")
	FInputActionValue FrameActionValue_OnLookMouse;

	UPROPERTY(BlueprintReadWrite, Category = "LoopEffect")
	FInputActionValue FrameActionValue_OnLook;

	UPROPERTY(BlueprintReadWrite, Category = "LoopEffect")
	FInputActionValue FrameActionValue_OnMove;

	UPROPERTY(BlueprintReadWrite, Category = "LoopEffect")
	FInputActionValue FrameActionValue_OnSprint;

	UPROPERTY(BlueprintReadWrite, Category = "LoopEffect")
	FInputActionValue FrameActionValue_OnJump;

	UPROPERTY(BlueprintReadWrite, Category = "LoopEffect")
	FInputActionValue FrameActionValue_OnAim;
};