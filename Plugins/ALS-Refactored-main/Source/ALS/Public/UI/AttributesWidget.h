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

	UFUNCTION(BlueprintImplementableEvent, Category = "Attributes")
	void SetStrengthPercent(float Strength, float MaxStrength);

	UFUNCTION(BlueprintImplementableEvent, Category = "Attributes")
	void SetEndurancePercent(float Endurance, float MaxEndurance);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Attributes")
	void SetVitalityPercent(float Vitality, float MaxVitality);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Attributes")
	void SetAgilityPercent(float Agility, float MaxAgility);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Attributes")
	void SetDexterityPercent(float Dexterity, float MaxDexterity);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Attributes")
	void SetPerceptionPercent(float Perception, float MaxPerception);

	void InitWithCharacterOwner(AAlsCharacter* PlayerCharacter);
};
