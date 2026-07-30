#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DA_Recipes.generated.h"

USTRUCT(BlueprintType)
struct FRecipeIngredient
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	FName IngredientName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	int32 IngredientQuantity;
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
	TSubclassOf<AA_Cookable> ResultCookableObject;
};

UCLASS()
class ALSEXTRAS_API UDA_Recipes : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	TArray<FRecipe> Recipes;
};
