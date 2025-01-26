#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Components/ActorComponent.h"
#include "InteractiveItemComponent.h"
#include "InteractivePickerComponent.generated.h"

// Делегаты для событий взаимодействия
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractiveFocusEvent, UInteractiveItemComponent*, FocusedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractiveEvent, UInteractiveItemComponent*, UseInteractiveComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractiveLostFocusEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPickerStartUsePressKeyEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPickerEndHoldUseEvent);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPSKITALSREFACTORED_API UInteractivePickerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractivePickerComponent();

protected:
	virtual void BeginPlay() override;

public:
	FORCEINLINE float GetDept() const { return Depth; }

	UFUNCTION()
	void SetCurrentItem(UInteractiveItemComponent* FoundItem);

	UFUNCTION(BlueprintCallable, Category = "InteractiveItem")
	void ResetCurrentItem();

private:
	void TickPicker(float DeltaTime);

	UInteractiveItemComponent* TraceNearestUsableObject(const FVector& Location, const FVector& Direction, TMap<AActor*, FString>& OutTracedActors,
		const TArray<AActor*>& ActorsToIgnore) const;

	UFUNCTION()
	void TickSetCurrentItem(UInteractiveItemComponent* FoundItem);

	UFUNCTION()
	void LostComponentNow(AActor* Owner, UInteractiveItemComponent* InteractiveComponent);

	UFUNCTION()
	void FoundComponentNow(AActor* Owner, UInteractiveItemComponent* InteractiveComponent);

public:
	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem")
	FOnInteractiveFocusEvent OnInteractiveFocusEvent;

	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem")
	FOnInteractiveLostFocusEvent OnInteractiveLostFocusEvent;

	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem")
	FInteractiveEvent OnInteractiveSelected;

	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem")
	FPickerStartUsePressKeyEvent OnPickerStartUsePressKeyEvent;

	UFUNCTION(BlueprintCallable, Category = "InteractiveItem")
	UInteractiveItemComponent* DoInteractiveUse();

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