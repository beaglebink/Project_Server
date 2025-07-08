#pragma once

#include "Components/TimelineComponent.h"
#include "I_OnInventoryClose.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_BaseInteractiveContainer.generated.h"

class UInteractiveItemComponent;
class UInteractivePickerComponent;
class UAC_Container;

UCLASS()
class ALSEXTRAS_API AA_BaseInteractiveContainer : public AActor, public II_OnInventoryClose
{
	GENERATED_BODY()

public:
	AA_BaseInteractiveContainer();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* SkeletalMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UInteractiveItemComponent* InteractiveComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAC_Container> ContainerComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UAudioComponent* AudioComponent;

protected:
	UPROPERTY(BlueprintReadWrite, Category = "Interaction")
	uint8 bIsOpen : 1 {false};

	uint8 bInProcess : 1 {false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lid")
	USoundBase* OpenSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Lid")
	USoundBase* CloseSound;

protected:
	UPROPERTY()
	UTimelineComponent* OpenTimeline;

	UPROPERTY(EditDefaultsOnly, Category = "Timeline")
	UCurveFloat* FloatCurve;

	FOnTimelineFloat ProgressFunction;

	FOnTimelineEvent FinishedFunction;

	UFUNCTION()
	virtual void TimelineProgress(float Value);

	UFUNCTION()
	virtual void TimelineFinished();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void OpenClose(UInteractivePickerComponent* Picker);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void Open(UInteractivePickerComponent* Picker);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void Close();

	virtual void OnCloseInventoryEvent_Implementation()override;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void OnLostFocus(ACharacter* Character);
};
