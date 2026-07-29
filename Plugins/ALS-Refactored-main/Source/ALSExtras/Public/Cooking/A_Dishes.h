#pragma once

#include "CoreMinimal.h"
#include "PythonContainers/A_InteractableActor.h"
#include "Components/TimelineComponent.h"
#include "Cooking/DA_Recipes.h"
#include "A_Dishes.generated.h"

UENUM(BlueprintType)
enum class EHeatingLevel : uint8
{
	None	UMETA(DisplayName = "No heating"),
	Low		UMETA(DisplayName = "Low level heating"),
	Medium	UMETA(DisplayName = "Medium level heating"),
	High	UMETA(DisplayName = "High level heating")
};

class USphereComponent;
class AA_Cookable;

UCLASS()
class ALSEXTRAS_API AA_Dishes : public AA_InteractableActor
{
	GENERATED_BODY()


public:
	AA_Dishes();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	virtual void Destroyed() override;

private:
	uint8 bIsOnAttaching : 1{false};

	UPROPERTY()
	TArray<AA_Cookable*> Ingredients;

	UPROPERTY()
	TMap<FName, int32> IngredientCountMap;

	TSet<AActor*> OverlappingActors;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* CollisionShape;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking")
	USkeletalMeshComponent* AttachedMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking")
	FName AttachedSocketName;

	uint8 bIsPlacing : 1{false};

	FTimerHandle AttachTimerHandle;
	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void AttachDishToHand(ACharacter* PlayerCharacter, FName SocketName);

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void DetachDishFromHand();

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void RotateDish(float AngleDelta);

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void TossDish(float Delta);

	FTimerHandle TossTimerHandle;

private:
	float OnPlacingPauseCheckTime = 0.0f;

	float PreviousAngle = 0.0f;

	float TossOffset = 0.0f;

	uint8 bIsTossing : 1 {false};


	FVector PrevLocation = FVector::ZeroVector;
	FVector CurrentLocation = FVector::ZeroVector;
	float DeltaLengthAccum = 0.0f;

protected:
	//Toss Timeline
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* TossTimeline;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* TossLocationFloatCurve;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* TossRotationFloatCurve;

	FOnTimelineFloat TossLocationProgressFunction;

	FOnTimelineFloat TossRotationProgressFunction;

	FOnTimelineEvent TossFinishedFunction;

	UFUNCTION()
	void TossLocationTimelineProgress(float Value);

	UFUNCTION()
	void TossRotationTimelineProgress(float Value);

	UFUNCTION()
	void TossTimelineFinished();

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking")
	UDA_Recipes* Recipes;

private:
	EHeatingLevel HeatingLevel;

	FTimerHandle CookingTimerHandle;

public:
	void SetHeatingLevel(EHeatingLevel NewLevel);

	EHeatingLevel GetHeatingLevel();

	//void CheckIfCooked();
};
