// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Interfaces/I_PluginToProject.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_WindController.generated.h"

class UArrowComponent;
class UTextRenderComponent;

UCLASS()
class FPSKITALSREFACTORED_API AA_WindController : public AActor, public II_PluginToProject
{
	GENERATED_BODY()

public:
	AA_WindController();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Speed", meta = (ClampMin = 0.0, ClampMax = 2500.0, ForceUnits = "cm/s"))
	float WindSpeed;

	virtual FVector2D GetWindDirectionAndSpeed()override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UArrowComponent* ArrowComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTextRenderComponent* TextRenderComponent;


public:
	virtual void Tick(float DeltaTime) override;
};
