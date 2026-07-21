#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Cooking/A_Dishes.h"
#include "I_CookingInteraction.generated.h"

UINTERFACE(MinimalAPI)
class UI_CookingInteraction : public UInterface
{
	GENERATED_BODY()
};

class ALSEXTRAS_API II_CookingInteraction
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CookingInterface")
	void SetDishes(AA_Dishes* Dish);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CookingInterface")
	AA_Dishes* GetDishes();
};
