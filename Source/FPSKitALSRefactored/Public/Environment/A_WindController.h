// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/WindDirectionalSource.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_WindController.generated.h"

class UTextRenderComponent;

UCLASS(Blueprintable)
class FPSKITALSREFACTORED_API AA_WindController : public AWindDirectionalSource
{
	GENERATED_BODY()

public:
	AA_WindController();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UArrowComponent* WindArrow;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTextRenderComponent* TextRenderComponent;


public:
	virtual void Tick(float DeltaTime) override;
};
