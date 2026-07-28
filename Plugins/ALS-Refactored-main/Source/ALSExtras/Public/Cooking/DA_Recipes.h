#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DA_Recipes.generated.h"

USTRUCT(BlueprintType)
struct FRecipeIngredient
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	FName IngredientName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	int32 IngredientQuantity;
};

USTRUCT(BlueprintType)
struct FRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	FName RecipeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	TArray<FRecipeIngredient> Ingredients;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	UStaticMesh* ResultMesh;
};

UCLASS()
class ALSEXTRAS_API UDA_Recipes : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	TArray<FRecipe> Recipes;
};
