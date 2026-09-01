#pragma once

#include "CoreMinimal.h"
#include "PythonContainers/A_InteractableActor.h"
#include "Components/TimelineComponent.h"
#include "Cooking/DA_Recipes.h"
#include "CoreBlueprintFunctionLibrary.h"
#include "A_Dishes.generated.h"

UENUM(BlueprintType)
enum class EHeatingLevel : uint8
{
	None	UMETA(DisplayName = "No heating"),
	Low		UMETA(DisplayName = "Low level heating"),
	Medium	UMETA(DisplayName = "Medium level heating"),
	High	UMETA(DisplayName = "High level heating")
};

USTRUCT(BlueprintType)
struct FPourEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour")
	ELiquidType LiquidType = ELiquidType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Pour")
	float 	AmountAdded = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Pour")
	float TimeStart;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Pour")
	float TimeEnd;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Pour")
	float TimingScore = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Pour")
	float TimingQuality = 0.0f;
};

USTRUCT(BlueprintType)
struct FLiquidStepOnPour
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pour")
	TArray<FPourEvent> PourEvents;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Pour")
	float AmountQuality = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Pour")
	float 	WeightedTimingQuality = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Pour")
	float LiquidStepQuality = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Pour")
	float WeightedLiquidContribution = 0.0f;
};

USTRUCT()
struct FIngredientQuality
{
	GENERATED_BODY()

	float GroupQuality = 0.0f;
	float TotalQualityByWeight = 0.0f;
	float ChunksWeightTotal = 0.0f;
	float WorstChunkQuality = 1.0f;
	float WorstChunkInfluence = 0.0f;
	float MaxChunkWeight = 0.0f;
	float RecipeImportance = 1.0f;
	float FailureSeverity = 0.0f;
	float MissingProportion = 0.0f;
	float ShortageSeverity = 0.0f;
};


class USphereComponent;
class AA_Cookable;
class UNiagaraComponent;
class UDA_FluidPoints;
class UWidgetComponent;
class USpringArmComponent;

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

	TSet<AActor*> OverlappingPlates;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* CollisionShape;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USphereComponent* CheckForPlateSphere;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* LiquidShape;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* CookedResultSpawnPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* LiquidLevelPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* FullLiquidLevelPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* CentreLiquidLevelPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* EmptyLiquidLevelPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	USpringArmComponent* SpringArmToEdge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* EdgeLiquidPoint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UAudioComponent* AudioComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UWidgetComponent* DishWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	bool bShowCookingDebug = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	USkeletalMeshComponent* AttachedMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	FName AttachedSocketName;

	// Pouring settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PouringSettings", meta = (ClampMin = 0.0f, ClampMax = 179.0f))
	float RotateAngle = 135.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PouringSettings", meta = (ClampMin = 0.0f))
	float DefaultPauseTimeOnRotation = 5.0f;

	float PauseTimeOnRotation;

	float CurrentTiltAngle = 0.0f;
	float PreviousTiltAngle = 0.0f;

	UPROPERTY()
	float PourIntensityNormalized = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PouringSettings", meta = (ClampMin = 0.0f, ClampMax = 5.0f))
	float LiquidPourRateNormalized = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PouringSettings", meta = (ClampMin = 0.0f, DisplayName = "LiquidVolume (ml)"))
	float LiquidVolume = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PouringSettings", meta = (ClampMin = 1.0f))
	float MinTimeToStartNewPourEvent = 3.0f;

	UPROPERTY()
	float LiquidLevelNormalized = 1.0f;

	UPROPERTY()
	float FullLiquidLevel = 0.0f;

	UPROPERTY()
	float FullLiquidVolume = 0.0f;

	UPROPERTY()
	float BoundsRadius = 0.0f;

	void PourLiquid(float DeltaTime);

	// Placing settings
	uint8 bIsPlacing : 1{false};

	FTimerHandle AttachTimerHandle;
	UFUNCTION(BlueprintCallable, Category = "CookingSettings")
	void AttachDishToHand(ACharacter* PlayerCharacter, FName SocketName);

	UFUNCTION(BlueprintCallable, Category = "CookingSettings")
	void DetachDishFromHand();

	UFUNCTION(BlueprintCallable, Category = "CookingSettings")
	void RotateDish(float AngleDelta);

private:
	float OnPlacingPauseCheckTime = 0.0f;

	float PreviousAngle = 0.0f;

	EHeatingLevel HeatingLevel;

	FTimerHandle CookingTimerHandle;

	uint8 bIsOnRecipeChecking : 1{false};

	//Toss
	FVector PrevLocation = FVector::ZeroVector;

	FVector CurrentLocation = FVector::ZeroVector;

	FVector DeltaLocation = FVector::ZeroVector;

	float DeltaLengthAccum = 0.0f;

	//Check recipe
	FRecipe CheckedRecipe;

	TMap<FName, FIngredientQuality> IngredientQualityMap;

	float DishQuality = 0.0f;
	float LowestGroupQuality = 1.0f;
	float DishAverage = 0.0f;
	float MissingPieceDeduction = 0.0f;

	uint8 bPrevPlateState : 1{false};
	uint8 bCurrentPlateState : 1{false};
	FText PrevTooltipText;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking")
	AA_Dishes* Plate;

public:
	void SetHeatingLevel(EHeatingLevel NewLevel);

	EHeatingLevel GetHeatingLevel();

	bool CheckIfCooked();

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void ReplaceIngredientsByCookedFood();

	//Fluid simulation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fluid")
	UNiagaraComponent* FluidFX;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Fluid")
	UDA_FluidPoints* FluidPointsDataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fluid")
	FLinearColor LiquidColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fluid")
	ELiquidType LiquidType = ELiquidType::None;

	UPROPERTY()
	FVector CutPlaneNormal = FVector(0.0f, 0.0f, 1.0f);

	UFUNCTION(CallInEditor, Category = "Fluid")
	void UpdateNiagaraPreview();

	//Pour simulation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fluid")
	UNiagaraComponent* PourFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fluid")
	float SphereLocationRadius = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fluid")
	float PourVelocityValue = 200.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Fluid")
	TArray<FLiquidStepOnPour> LiquidStepsOnPour;

	UFUNCTION(BlueprintCallable, Category = "Fluid")
	void AddLiquid(ELiquidType Type, float Amount);

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingTimer")
	uint8 IsACookWare : 1{false};

private:
	FTimerHandle SteamSoundStopTimerHandle;

	uint8 bPrevHasIngredientsState : 1{false};

	uint8 bCurrentHasIngredientsState : 1{false};

	float CookingTimerValue = 0.0f;

	void UpdateCookingSession();

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "CookingTimer")
	void SetCookingTimerVisibility(bool IsSet);

	UFUNCTION(BlueprintImplementableEvent, Category = "CookingTimer")
	void SetCookingTimerValue(int Value);

	//Pour events registration
private:
	FRecipe CurrentRecipe = FRecipe();

	float TotalAmountAddedPerStep = 0.0f;

	float PrevPourTime = 0.0f;

	float CurrentPourTime = 0.0f;

	float CurrentBoundaryTime = 0.0f;

	ELiquidType LastLiquidType = ELiquidType::None;

	int32 CurrentStepIndexInRecipe = 0;

	float CalculateTimingQualityPerPourMoment(float CurrentTime, FLiquidStep CurrentRecipeStep);

	void ResetCookingSession();

	void UpdatePourVisual_TargetDish(ELiquidType Type, float LiquidAmount, FLiquidStep RecipeLiquidStep);
};