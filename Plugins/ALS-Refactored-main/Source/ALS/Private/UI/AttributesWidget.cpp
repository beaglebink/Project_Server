#include "UI/AttributesWidget.h"
#include "AlsCharacter.h"

void UAttributesWidget::InitWithCharacterOwner(AAlsCharacter* PlayerCharacter)
{
	PlayerCharacter->OnHealthChanged.AddDynamic(this, &UAttributesWidget::SetHealthPercent);
	PlayerCharacter->OnStaminaChanged.AddDynamic(this, &UAttributesWidget::SetStaminaPercent);

	SetHealthPercent(PlayerCharacter->GetHealth(), PlayerCharacter->GetMaxHealth());
	SetStaminaPercent(PlayerCharacter->GetStamina(), PlayerCharacter->GetMaxStamina());
}
