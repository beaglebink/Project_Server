#pragma once

#include "I_OnInventoryClose.h"
#include "Components/TimelineComponent.h"
#include "S_ItemData.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_Chest.generated.h"

class UInteractiveItemComponent;
class UInteractivePickerComponent;
class UAC_Container;

UCLASS()
class ALSEXTRAS_API AA_Chest : public AActor, public II_OnInventoryClose
{
	GENERATED_BODY()
	
public:	
	AA_Chest();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* SkeletalMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UInteractiveItemComponent* InteractiveComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAC_Container> ContainerComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UAudioComponent* AudioComponent;

private:
	UPROPERTY()
	UTimelineComponent* OpenTimeline;

protected:
	FOnTimelineFloat ProgressFunction;

	FOnTimelineEvent FinishedFunction;

	UFUNCTION()
	void TimelineProgress(float Value);

	UFUNCTION()
	void TimelineFinished();

protected:
	UPROPERTY(BlueprintReadWrite, Category = "Interaction")
	uint8 bIsOpen : 1 {false};

	uint8 bInProcess : 1 {false};

	UPROPERTY(EditDefaultsOnly, Category = "Timeline")
	UCurveFloat* FloatCurve;

	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintReadOnly, Category = "Lid")
	float OpenAngle;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = "Lid")
	USoundBase* OpenSound;

	UPROPERTY(EditDefaultsOnly,BlueprintReadWrite, Category = "Lid")
	USoundBase* CloseSound;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void OpenCloseChest(UInteractivePickerComponent* Picker);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void OpenChest(UInteractivePickerComponent* Picker);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void CloseChest();

	virtual void OnCloseInventoryEvent_Implementation()override;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void OnLostFocus(ACharacter* Character);
};
