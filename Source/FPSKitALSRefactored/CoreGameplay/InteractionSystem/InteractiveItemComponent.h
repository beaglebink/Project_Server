// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "InteractiveItemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractivePicker, UInteractivePickerComponent*, Picker);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractiveUseEvent, ACharacter*, User);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEndHoldUseEvent, AActor*, Initiator);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStartUseEvent, ACharacter*, Initiator);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractiveNow, AActor*, WhoInteract, bool, IsInteractiveNow);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FPSKITALSREFACTORED_API UInteractiveItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInteractiveItemComponent();

	void CallInteractiveSelected(AActor* Owner);

	UFUNCTION()
	void FinishInteractiveUse(ACharacter* IIUser, const bool IsReleaseButton = true);

	void SetIsInteractiveNow(AActor* WhoInteract, bool Value);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY(BlueprintAssignable)
	FInteractivePicker OnInteractorSelected;

	UPROPERTY(BlueprintAssignable)
	FOnInteractiveUseEvent OnInteractiveFinishUseEvent;

	UPROPERTY(BlueprintAssignable)
	FEndHoldUseEvent OnUseReleaseKeyEvent;

	UPROPERTY(BlueprintAssignable)
	FStartUseEvent OnStartUsePressKeyEvent;

	UPROPERTY(BlueprintAssignable)
	FOnInteractiveNow OnInteractiveNow;

private:
	ACharacter* ReleasedUser;
	bool IsRelease;
	bool IsInteractiveNow;
};
