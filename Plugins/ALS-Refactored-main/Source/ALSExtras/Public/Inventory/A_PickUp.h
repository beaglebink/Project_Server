#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SaveSubsystems/I_SaveableObject.h"
#include "Inventory/O_PickUpStructures.h"
#include "A_PickUp.generated.h"

class UInteractiveItemComponent;
class UInteractivePickerComponent;
class UFloorAssignmentComponent;

UCLASS(BlueprintType, Blueprintable)
class ALSEXTRAS_API AA_PickUp : public AActor
{
	GENERATED_BODY()

public:
	AA_PickUp();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* StaticMeshComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UInteractiveItemComponent* InteractiveComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UFloorAssignmentComponent* FloorAssignmentComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	USoundBase* Sound;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void AddToInventory(UInteractivePickerComponent* Picker);

public:
	// Data saving
	UFUNCTION(BlueprintCallable, Category = "Saving")
	FString CollectSaveData();

	UFUNCTION(BlueprintCallable, Category = "Saving")
	void ApplySaveData(FString Jsonstring);
};
