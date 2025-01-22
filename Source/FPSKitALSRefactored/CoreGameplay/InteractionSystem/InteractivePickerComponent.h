// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Components/ActorComponent.h"
#include "InteractiveItemComponent.h"
#include "InteractivePickerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractiveFocusEvent, UInteractiveItemComponent*, FocusedItem);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractiveEvent, UInteractiveItemComponent*, UseInteractiveComponent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractiveLostFocusEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPickerStartUsePressKeyEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPickerEndHoldUseEvent);

/*
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPickerStartUsePressKeyEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPickerBeginHoldUseEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPickerRepeatUseEvent, float, Progress);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPickerEndHoldUseEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPickerUseReleaseKeyEvent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPickerFailedHoldUseEvent);



DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInteractiveActorReason, UInteractiveItemComponent*, UseInteractiveComponent, EReason, Reason);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInteractiveActorProgress, UInteractiveItemComponent*, UseInteractiveComponent, float, Progress);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FShowInteractionTrace, AActor*, Actor, bool, IsInteractable);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShowTracedActors, const TArray<FTracedActorsInfo>&, States);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FShowGameplayTags, const TArray< FGameplayTag>&, PlayerTags, const TArray< FGameplayTag>&, RequiredTags, const TArray< FGameplayTag>&, BlockingTags, bool, IsBlock);
*/

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FPSKITALSREFACTORED_API UInteractivePickerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInteractivePickerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE float GetPickRadius() const { return Depth; }

	UFUNCTION()
	void SetCurrentItem(UInteractiveItemComponent* FoundItem, bool IsPeriodicalUpdate);

	UFUNCTION()
	void OnStartUsePressKeyEvent(ACharacter* Character);

	UFUNCTION()
	void OnUseReleaseKeyEvent(AActor* Initiator);

private:
	void TickPicker(float DeltaTime);

	UInteractiveItemComponent* TraceNearestUsableObject(const FVector& Location, const FVector& Direction, TMap<AActor*, FString>& OutTracedActors, float Radius,
		const TArray<AActor*>& ActorsToIgnore) const;

	UFUNCTION()
	void TickSetCurrentItem(UInteractiveItemComponent* FoundItem);

	UFUNCTION()
	void LostComponentNow(AActor* Owner, UInteractiveItemComponent* InteractiveComponent, bool IsPeriodicalUpdate);

	UFUNCTION()
	void FoundComponentNow(AActor* Owner, UInteractiveItemComponent* InteractiveComponent, bool IsPeriodicalUpdate);


public:
	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem")
	FOnInteractiveFocusEvent OnInteractiveFocusEvent;

	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem")
	FInteractiveEvent	OnInteractiveRemoved;

	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem")
	FOnInteractiveLostFocusEvent OnInteractiveLostFocusEvent;

	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem")
	FInteractiveEvent	OnInteractiveSelected;

	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem")
	FPickerStartUsePressKeyEvent OnPickerStartUsePressKeyEvent;

	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem")
	FInteractiveEvent	OnInteractionStarted;

	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem")
	FPickerEndHoldUseEvent OnPickerEndHoldUseEvent;

	UPROPERTY(Category = "TheGame|InteractiveItem", EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool DebugDraw = true;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	bool IsShowInteractionTrace = false;

	UPROPERTY(Category = "TheGame|InteractiveItem", EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float PickTickInterval = 0.3f;

	UPROPERTY(Category = "TheGame|InteractiveItem", EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float Depth = 300.f;

	UPROPERTY(Category = "TheGame|InteractiveItem", EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float Width = 35.f;

	UPROPERTY()
	UInteractiveItemComponent* CurrentItem = nullptr;

private:
	FTimerDelegate TimerDel;
	FTimerHandle TimerHandle;

	UPROPERTY()
	TMap<AActor*, FString> TracedActors;

	UPROPERTY()
	TArray<AActor*> FoundCharacters;

	UPROPERTY()
	TArray<AActor*> ActorsToIgnoreCache;

	bool CurrentIItemIsValid = false;
		
};
