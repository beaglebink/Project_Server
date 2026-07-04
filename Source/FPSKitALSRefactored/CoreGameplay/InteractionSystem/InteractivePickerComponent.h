#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Components/ActorComponent.h"
#include "InteractiveItemComponent.h"
#include "InteractivePickerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractiveFocusEvent, UInteractiveItemComponent*, FocusedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInteractiveLostFocusEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPickerStartUsePressKeyEvent);
// OnInteractTooltipChange delegate is declared in InteractiveItemComponent.h
// (Делегат OnInteractTooltipChange объявлен в InteractiveItemComponent.h)
// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractTooltipChange, const FText&, NewTooltip);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPSKITALSREFACTORED_API UInteractivePickerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractivePickerComponent();

protected:
	virtual void BeginPlay() override;

public:
	FORCEINLINE float GetDepth() const { return Depth; }

	UFUNCTION()
	void SetCurrentItem(UInteractiveItemComponent* FoundItem);

	void CheckFoundComponent(AActor* Owner, UInteractiveItemComponent* InteractiveComponent);

	UFUNCTION()
	void StartFoundComponent();

	UFUNCTION()
	void StartRejectComponent();

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

    // Helper to handle tooltip change coming from interactive item component.
    // Must be UFUNCTION for AddDynamic
    // (Обработчик изменения тултипа — должен быть UFUNCTION для AddDynamic)
    UFUNCTION()
    void HandleInteractTooltipChange(const FText& NewTooltip);

public:
	UPROPERTY(BlueprintAssignable, SaveGame, Category = "InteractiveItem")
	FOnInteractiveFocusEvent OnInteractiveReceiveFocusEvent;

	UPROPERTY(BlueprintAssignable, SaveGame, Category = "InteractiveItem")
	FOnInteractiveLostFocusEvent OnInteractiveLostFocusEvent;

	UPROPERTY(BlueprintAssignable, SaveGame, Category = "InteractiveItem")
	FPickerStartUsePressKeyEvent OnInteractionPressKeyEvent;

	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem")
    // Uses the tooltip delegate declared in InteractiveItemComponent.h
    // (Использует делегат, объявленный в InteractiveItemComponent.h)
    FOnInteractTooltipChange OnInteractTooltipChange;

    // Fired when subsystem updates enabled state or tooltip
    // (Срабатывает, когда подсистема обновляет enabled или тултип)
	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem")
	FOnInteractStateChanged OnInteractStateChanged;

	UFUNCTION(BlueprintCallable, Category = "InteractiveItem")
	UInteractiveItemComponent* DoInteractiveUse();

	UPROPERTY(Category = "TheGame|InteractiveItem", EditDefaultsOnly, SaveGame, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool DebugDraw = false;

	UPROPERTY(Category = "TheGame|InteractiveItem", EditDefaultsOnly, SaveGame, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float PickTickInterval = 0.3f;

	UPROPERTY(Category = "TheGame|InteractiveItem", EditDefaultsOnly, SaveGame, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float Depth = 300.f;

	UPROPERTY(Category = "TheGame|InteractiveItem", EditDefaultsOnly, SaveGame, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float Width = 35.f;

	UPROPERTY(BlueprintReadOnly, Category = "InteractiveComponent")
	UInteractiveItemComponent* CurrentItem = nullptr;

private:
	FTimerDelegate TimerDel;

	UPROPERTY(SaveGame)
	FTimerHandle TimerHandle;

	UPROPERTY()
	TMap<AActor*, FString> TracedActors;

	UPROPERTY()
	TArray<AActor*> FoundCharacters;

	UPROPERTY()
	TArray<AActor*> ActorsToIgnoreCache;

	UPROPERTY(SaveGame)
	bool CurrentIItemIsValid = false;

	UPROPERTY()
	AActor* StoredOwner = nullptr;

	UPROPERTY()
	UInteractiveItemComponent* StoredInteractiveComponent = nullptr;
};