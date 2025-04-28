// Fill out your copyright notice in the Description page of Project Settings.


#include "Environment/A_WindController.h"
#include "Components/ArrowComponent.h"
#include "Components/TextRenderComponent.h"

AA_WindController::AA_WindController()
{
	WindArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("WindArrow"));
	TextRenderComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextComponent"));

	WindArrow->SetupAttachment(RootComponent);
	TextRenderComponent->SetupAttachment(WindArrow);
	
	WindArrow->SetArrowFColor(FColor::Red);
	WindArrow->SetArrowSize(2.0f);
	TextRenderComponent->SetText(FText::FromString("Wind direction"));
	TextRenderComponent->SetHorizontalAlignment(EHTA_Center);
	TextRenderComponent->SetWorldSize(30.0f);

	Tags.Add(TEXT("WindController"));
}

void AA_WindController::BeginPlay()
{
	Super::BeginPlay();

}

void AA_WindController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

