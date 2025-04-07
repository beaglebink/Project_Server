// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AttributesWidget.generated.h"

UCLASS()
class ALS_API UAttributesWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Attributes")
	void SetHealthPercent(float HealthPercent);

	UFUNCTION(BlueprintImplementableEvent, Category = "Attributes")
	void SetStaminaPercent(float StaminaPercent);

protected:
	virtual void NativeConstruct() override;
};
