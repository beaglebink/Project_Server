// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractItemRegistrationPayload.h"
#include "InteractItemStatePayload.h"
#include "EventBusSubsystem.h"
#include "InteractiveItemComponent.generated.h"

class UOutcomeConditionAsset;
class UInteractivePickerComponent;

// How long the interaction button must be held
UENUM(BlueprintType)
enum class EInteractDuration : uint8
{
	Instant  = 0 UMETA(DisplayName = "Instant"),
	Continue UMETA(DisplayName = "Hold")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractivePicker, UInteractivePickerComponent*, Picker);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractiveUseEvent, ACharacter*, User);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractiveNow, AActor*, WhoInteract);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractStateChanged, bool, bEnabled, const FText&, NewTooltip);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FPSKITALSREFACTORED_API UInteractiveItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractiveItemComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Called by Picker when this item comes into trace range
	void SetIsInteractiveNow(AActor* WhoInteract);

	// Called by Picker when this item leaves trace range
	UFUNCTION()
	void FinishInteractiveUse(ACharacter* IIUser, const bool IsReleaseButton = true);

	// Called by Picker on interaction button press
	void DoInteractiveUse(ACharacter* IIUser);

	// Убираем проблему линковки — делаем реализацию inline в заголовке.
	// Можно вызывать и из C++ и из BP.
	UFUNCTION(BlueprintCallable, Category = "InteractiveItem")
	void SetIsActive(bool Active) { SetActive(Active); }

	// Returns unique ID of this item - used in all EventBus messages
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractiveItem")
	FGuid GetItemId() const { return ItemId; }

	// Apply state update received from subsystem via EventBus
	UFUNCTION(BlueprintCallable, Category = "InteractiveItem")
	void ApplyStateFromPayload(const UInteractItemStatePayload* Payload);

	// Called by subsystem after it processed registration (notification)
	UFUNCTION()
	void OnRegisteredBySubsystem(UInteractItemRegistrationPayload* Payload);

	// Called by subsystem after it processed unregistration (notification)
	UFUNCTION()
	void OnUnregisteredBySubsystem(UInteractItemRegistrationPayload* Payload);

protected:
	// Legacy: handler when subsystem publishes InteractEnabled via EventBus (kept)
	void OnInteractEnabledOutcome(const FOutcomeEventBase& Outcome);

public:
	// Events
	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem|Events")
	FOnInteractiveNow OnInteractiveReceiveFocusEvent;

	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem|Events")
	FOnInteractiveUseEvent OnInteractiveLostFocusEvent;

	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem|Events")
	FInteractivePicker OnInteractionPressKeyEvent;

	// Fired when subsystem updates enabled state or tooltip
	UPROPERTY(BlueprintAssignable, Category = "InteractiveItem|Events")
	FOnInteractStateChanged OnInteractStateChanged;

	// Config
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractiveItem|Config")
	EInteractiveSubsystem SubsystemType = EInteractiveSubsystem::Interior;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractiveItem|Config")
	float InteractionRange = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractiveItem|Config")
	FText InteractiveTooltipText;

	// Hold or instant interaction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractiveItem|Config")
	EInteractDuration InteractDuration = EInteractDuration::Instant;

	// Dragging transform offsets (restored)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractiveItem|Config")
	FVector DraggingLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractiveItem|Config")
	FRotator DraggingRotator = FRotator::ZeroRotator;

private:
	// Auto-generated unique ID at BeginPlay
	FGuid ItemId;

	// Current state - maintained by subsystem updates via EventBus
	bool bInteractionEnabled = true;
	FText CurrentTooltip;

	UPROPERTY()
	ACharacter* ReleasedUser = nullptr;

	bool IsRelease = false;
	bool IsInteractiveNow = false;
};
