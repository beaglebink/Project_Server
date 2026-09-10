#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NativeGameplayTags.h"
#include "DA_Recipes.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooking_Ingredients);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooking_Liquids);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooking_Spices);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooking_WokMovements);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooking_Intervals);

USTRUCT(BlueprintType)
struct FRecipeIngredient
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	FGameplayTag IngredientTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0))
	int32 TargetChunkCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0))
	int32 IdealChunkCountMin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0))
	int32 IdealChunkCountMax;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.1f, ClampMax = 100.0f))
	float CountTolerancePercent = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.0f))
	float TargetChunkSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.1f, ClampMax = 100.0f))
	float IdealSizeTolerancePercent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.1f, ClampMax = 100.0f))
	float MaximumSizeTolerancePercent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.1f, ClampMax = 100.0f))
	float IdealEvennessTolerancePercent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.1f, ClampMax = 100.0f))
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
	FGameplayTag LiquidTag;

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
};

USTRUCT(BlueprintType)
struct FSpicesStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	FGameplayTag SpicesTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "EarlyTolerance (s)"))
	float EarlyTolerance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "LateTolerance (s)"))
	float LateTolerance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f))
	float TimingScoreWeight = 0.35f;
};

USTRUCT(BlueprintType)
struct FWokMovementStep
{
	GENERATED_BODY()

	int32 WokMovementQuantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0))
	int32 WokMovementQuantityMin = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0))
	int32 WokMovementQuantityMax = 0;
};

USTRUCT(BlueprintType)
struct FIntervalStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	FGameplayTag StartEvent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	FGameplayTag EndEvent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f))
	float IntervalDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "EarlyTolerance (s)"))
	float EarlyTolerance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, DisplayName = "LateTolerance (s)"))
	float LateTolerance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f))
	float TimingScoreWeight = 0.35f;
};

UENUM()
enum class ERecipeRequirementType : uint8
{
	None,
	Ingredient,
	Liquid,
	Spice,
	WokMovement,
	Interval
};

USTRUCT(BlueprintType)
struct FRecipeRequirement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	FGameplayTag RequirementTag;

	UPROPERTY()
	ERecipeRequirementType RequirementType = ERecipeRequirementType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, EditCondition = "RequirementType != ERecipeRequirementType::Liquid && RequirementType != ERecipeRequirementType::Interval", EditConditionHides))
	float RequirementStartTime = 0.0f;
	float RequirementEndTime = 0.0f;

	uint8 bChecked : 1{false};

	float StepQuality = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f))
	float StepRecipeImportance = 0.0f;

	// Requirement - Ingredient
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (EditCondition = "RequirementType == ERecipeRequirementType::Ingredient", EditConditionHides))
	FRecipeIngredient IngredientStep;

	// Requirement - Liquid
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (EditCondition = "RequirementType == ERecipeRequirementType::Liquid", EditConditionHides))
	FLiquidStep LiquidStep;

	// Requirement - Spice
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (EditCondition = "RequirementType == ERecipeRequirementType::Spice", EditConditionHides))
	FSpicesStep SpicesStep;

	// Requirement - Wok Movement
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (EditCondition = "RequirementType == ERecipeRequirementType::WokMovement", EditConditionHides))
	FWokMovementStep WokMovementStep;

	// Requirement - Interval
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (EditCondition = "RequirementType == ERecipeRequirementType::Interval", EditConditionHides))
	FIntervalStep IntervalStep;

	FORCEINLINE bool operator==(const FRecipeRequirement& Other) const
	{
		return RequirementTag.IsValid() && RequirementTag == Other.RequirementTag;
	}
};

class AA_Cookable;

USTRUCT(BlueprintType)
struct FRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	TArray<FRecipeRequirement> Requirements;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	TSubclassOf<AA_Cookable> ResultCookableClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	FDishQualityThresholds RatingThresholds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 5.0f))
	float SignificantChunkPercentage = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float FailureSeverityThreshold = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float MissingPenaltyStrength = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float ExtraRecipeIngredientPenaltyStrength = 0.85f;

	FRecipeIngredient* FindIngredientByTag(const FGameplayTag& IngredientTag)
	{
		for (FRecipeRequirement& Requirement : Requirements)
		{
			if (Requirement.RequirementTag.MatchesTag(TAG_Cooking_Ingredients) && Requirement.IngredientStep.IngredientTag == IngredientTag)
			{
				return &Requirement.IngredientStep;
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
			for (FRecipeRequirement& Requirement : Recipe.Requirements)
			{
				if (Requirement.RequirementTag.MatchesTag(TAG_Cooking_Ingredients))
				{
					Requirement.RequirementType = ERecipeRequirementType::Ingredient;
					Requirement.IngredientStep.UpdatePreparationImportanceInfo();
				}
				else if (Requirement.RequirementTag.MatchesTag(TAG_Cooking_Liquids))
				{
					Requirement.RequirementType = ERecipeRequirementType::Liquid;
				}
				else if (Requirement.RequirementTag.MatchesTag(TAG_Cooking_Spices))
				{
					Requirement.RequirementType = ERecipeRequirementType::Spice;
				}
				else if (Requirement.RequirementTag.MatchesTag(TAG_Cooking_WokMovements))
				{
					Requirement.RequirementType = ERecipeRequirementType::WokMovement;
				}
				else if (Requirement.RequirementTag.MatchesTag(TAG_Cooking_Intervals))
				{
					Requirement.RequirementType = ERecipeRequirementType::Interval;
				}
			}
		}
	}

#endif
};