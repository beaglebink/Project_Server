#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AttributesWidget.generated.h"

class AAlsCharacter;

UCLASS()
class ALS_API UAttributesWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Attributes")
	void SetHealthPercent(float Health, float MaxHealth);

	UFUNCTION(BlueprintImplementableEvent, Category = "Attributes")
	void SetStaminaPercent(float Stamina, float MaxStamina);

	void InitWithCharacterOwner(AAlsCharacter* PlayerCharacter);
};
