// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "I_PluginToProject.generated.h"

UINTERFACE(MinimalAPI)
class UI_PluginToProject : public UInterface
{
	GENERATED_BODY()
};

class ALS_API II_PluginToProject
{
	GENERATED_BODY()

public:
	virtual FVector2D GetWindDirectionAndSpeed() = 0;
};
