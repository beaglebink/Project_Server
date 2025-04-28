// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/A_WindController.h"
#include "Components/ArrowComponent.h"
#include "Components/TextRenderComponent.h"

AA_WindController::AA_WindController()
{
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	TextRenderComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextComponent"));

	ArrowComponent->SetupAttachment(RootComponent);
	TextRenderComponent->SetupAttachment(ArrowComponent);

	ArrowComponent->bHiddenInGame = false;

	TextRenderComponent->SetText(FText::FromString("Wind direction"));
	TextRenderComponent->SetHorizontalAlignment(EHTA_Center);
	TextRenderComponent->SetWorldSize(30.0f);

	Tags.Add(TEXT("WindController"));
}

void AA_WindController::BeginPlay()
{
	Super::BeginPlay();
	
}

FVector2D AA_WindController::GetWindDirectionAndSpeed()
{
	return FVector2D(GetActorForwardVector() * WindSpeed);
}

void AA_WindController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

