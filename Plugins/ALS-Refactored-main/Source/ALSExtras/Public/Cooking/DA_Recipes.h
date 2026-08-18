#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DA_Recipes.generated.h"

UENUM(BlueprintType)
enum class ELiquidType : uint8
{
	None	UMETA(DisplayName = "No Liquid"),
	Water	UMETA(DisplayName = "Water"),
	Broth	UMETA(DisplayName = "Broth"),
	Oil		UMETA(DisplayName = "Oil"),
	Sauce	UMETA(DisplayName = "Sauce")
};

USTRUCT(BlueprintType)
struct FRecipeIngredient
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	FName IngredientName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	int32 IngredientQuantity;
};

USTRUCT(BlueprintType)
struct FDishQualityThresholds
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float AcceptableThreshold = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float GoodThreshold = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float ExcellentThreshold = 0.8f;
};

USTRUCT(BlueprintType)
struct FLiquidStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	ELiquidType LiquidType = ELiquidType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "TargetAmount (ml)"))
	float TargetAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "IdealMinimumAmount (ml)"))
	float IdealMinimumAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "IdealMaximumAmount (ml)"))
	float IdealMaximumAmount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "IdealStartTime (s)"))
	float IdealStartTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "IdealEndTime (s)"))
	float IdealEndTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "EarlyTolerance (s)"))
	float EarlyTolerance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "LateTolerance (s)"))
	float LateTolerance = 0.0f;	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "AmountScoreWeight"))
	float AmountScoreWeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "TimingScoreWeight"))
	float TimingScoreWeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "RecipeImportance"))
	float RecipeImportance = 0.0f;
};


class AA_Cookable;

USTRUCT(BlueprintType)
struct FRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	FName RecipeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	TArray<FRecipeIngredient> Ingredients;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	TSubclassOf<AA_Cookable> ResultCookableClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	uint8 bRequiresToss : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	FDishQualityThresholds RatingThresholds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	TArray<FLiquidStep> LiquidSteps;
};

UCLASS()
class ALSEXTRAS_API UDA_Recipes : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	TArray<FRecipe> Recipes;
};
