#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NativeGameplayTags.h"
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0))
	int32 TargetChunkCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0))
	int32 IdealChunkCountMin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0))
	int32 IdealChunkCountMax;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.1f))
	float CountTolerancePercent = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.0f))
	float TargetChunkSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.1f))
	float IdealSizeTolerancePercent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.1f))
	float MaximumSizeTolerancePercent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.1f))
	float IdealEvennessTolerancePercent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.1f))
	float MaximumEvennessTolerancePercent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.0f))
	float CountScoreWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.0f))
	float SizeScoreWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.0f))
	float EvennessScoreWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float PreparationImportance = 1.0f;

	UPROPERTY(VisibleAnywhere, Category = "CookingSettings")
	FText PreparationImportanceLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	FText PreparationImportanceDescription;

#if WITH_EDITOR

	void UpdatePreparationImportanceInfo()
	{
		if (PreparationImportance <= 0.0f)
		{
			PreparationImportanceLevel = FText::FromString("None");
			PreparationImportanceDescription = FText::FromString("Cutting quality is not scored directly.");
		}
		else if (PreparationImportance <= 0.05f)
		{
			PreparationImportanceLevel = FText::FromString("Low");
			PreparationImportanceDescription = FText::FromString("Rough preparation is acceptable; only major mistakes matter.");
		}
		else if (PreparationImportance <= 0.10f)
		{
			PreparationImportanceLevel = FText::FromString("Moderate");
			PreparationImportanceDescription = FText::FromString("Good cutting helps, but cooking remains dominant.");
		}
		else if (PreparationImportance <= 0.15f)
		{
			PreparationImportanceLevel = FText::FromString("Important");
			PreparationImportanceDescription = FText::FromString("Size and consistency visibly matter to the dish.");
		}
		else
		{
			PreparationImportanceLevel = FText::FromString("High");
			PreparationImportanceDescription = FText::FromString("Use sparingly for recipes where uniform pieces are a meaningful preparation challenge.");
		}
	}

#endif
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "UnderAmountTolerance (ml)"))
	float UnderAmountTolerance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "OverAmountTolerance (ml)"))
	float OverAmountTolerance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "IdealStartTime (s)"))
	float IdealStartTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "IdealEndTime (s)"))
	float IdealEndTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "EarlyTolerance (s)"))
	float EarlyTolerance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "LateTolerance (s)"))
	float LateTolerance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f))
	float AmountScoreWeight = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f))
	float TimingScoreWeight = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f))
	float RecipeImportance = 0.0f;
};


class AA_Cookable;

USTRUCT(BlueprintType)
struct FRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	TArray<FRecipeIngredient> Ingredients;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	TSubclassOf<AA_Cookable> ResultCookableClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	uint8 bRequiresToss : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	FDishQualityThresholds RatingThresholds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 5.0f))
	float SignificantChunkPercentage = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float FailureSeverityThreshold = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float MissingPenaltyStrength = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	TArray<FLiquidStep> LiquidSteps;

	FRecipeIngredient* FindIngredientByName(const FName& IngredientName)
	{
		for (FRecipeIngredient& Ingredient : Ingredients)
		{
			if (Ingredient.IngredientName == IngredientName)
			{
				return &Ingredient;
			}
		}
		return nullptr;
	}
};

UCLASS()
class ALSEXTRAS_API UDA_Recipes : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	TMap<FGameplayTag, FRecipe> Recipes;

#if WITH_EDITOR

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);

		for (auto& [RecipeTag, Recipe] : Recipes)
		{
			for (FRecipeIngredient& Ingredient : Recipe.Ingredients)
			{
				Ingredient.UpdatePreparationImportanceInfo();
			}
		}
	}

#endif
};
