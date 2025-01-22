#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "InteractiveItemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractivePicker, UInteractivePickerComponent*, Picker);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractiveUseEvent, ACharacter*, User);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEndHoldUseEvent, AActor*, Initiator);

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FStartUseEvent, ACharacter*, Initiator);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractiveNow, AActor*, WhoInteract);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FPSKITALSREFACTORED_API UInteractiveItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInteractiveItemComponent();


	UFUNCTION()
	void FinishInteractiveUse(ACharacter* IIUser, const bool IsReleaseButton = true);

	void SetIsInteractiveNow(AActor* WhoInteract);

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(BlueprintAssignable)
	FInteractivePicker OnInteractorSelected;

	UPROPERTY(BlueprintAssignable)
	FOnInteractiveUseEvent OnInteractiveFinishUseEvent;

	UPROPERTY(BlueprintAssignable)
	FEndHoldUseEvent OnUseReleaseKeyEvent;
	/*
	UPROPERTY(BlueprintAssignable)
	FStartUseEvent OnStartUsePressKeyEvent;
	*/
	UPROPERTY(BlueprintAssignable)
	FOnInteractiveNow OnInteractiveNow;

private:
	ACharacter* ReleasedUser;
	bool IsRelease;
	bool IsInteractiveNow;
};
