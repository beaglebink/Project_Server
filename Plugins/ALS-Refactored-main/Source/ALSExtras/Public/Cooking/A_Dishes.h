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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* CookedResultSpawnPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	USkeletalMeshComponent* AttachedMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	FName AttachedSocketName;

	uint8 bIsPlacing : 1{false};

	FTimerHandle AttachTimerHandle;
	UFUNCTION(BlueprintCallable, Category = "CookingSettings")
	void AttachDishToHand(ACharacter* PlayerCharacter, FName SocketName);

	UFUNCTION(BlueprintCallable, Category = "CookingSettings")
	void DetachDishFromHand();

	UFUNCTION(BlueprintCallable, Category = "CookingSettings")
	void RotateDish(float AngleDelta);

	UFUNCTION(BlueprintCallable, Category = "CookingSettings")
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	UDA_Recipes* Recipes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = "0.0f", ClampMax = "1.0f"))
	float SignificantChunkPercentage = 0.1f;

private:
	EHeatingLevel HeatingLevel;

	FTimerHandle CookingTimerHandle;

	uint8 bIsOnRecipeChecking : 1{false};

public:
	void SetHeatingLevel(EHeatingLevel NewLevel);

	EHeatingLevel GetHeatingLevel();

	void CheckIfCooked();
};
