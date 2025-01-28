#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "InteractiveItemComponent.generated.h"

UENUM(BlueprintType)
enum class EInteractDuration : uint8
{
	Instant = 0,
	Continue
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractivePicker, UInteractivePickerComponent*, Picker);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractiveUseEvent, ACharacter*, User);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEndHoldUseEvent, AActor*, Initiator);

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

	void DoInteractiveUse(ACharacter* IIUser);

public:
	UPROPERTY(BlueprintAssignable)
	FInteractivePicker OnInteractorSelected;

	UPROPERTY(BlueprintAssignable)
	FOnInteractiveUseEvent OnInteractiveFinishUseEvent;

	UPROPERTY(BlueprintAssignable)
	FEndHoldUseEvent OnUseReleaseKeyEvent;

	UPROPERTY(BlueprintAssignable)
	FOnInteractiveNow OnInteractiveNow;

	UPROPERTY(BlueprintAssignable)
	FInteractivePicker OnInteractionStarted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractiveItem")
	FText InteractiveTooltipText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractiveItem")
	EInteractDuration InteractDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractiveItem")
	FVector DragingLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractiveItem")
	FRotator DragingRotator;

private:
	ACharacter* ReleasedUser;
	bool IsRelease;
	bool IsInteractiveNow;
};
