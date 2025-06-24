#include "UI/AttributesWidget.h"
#include "AlsCharacter.h"

void UAttributesWidget::InitWithCharacterOwner(AAlsCharacter* PlayerCharacter)
{
	PlayerCharacter->OnHealthChanged.AddDynamic(this, &UAttributesWidget::SetHealthPercent);
	PlayerCharacter->OnStaminaChanged.AddDynamic(this, &UAttributesWidget::SetStaminaPercent);
	PlayerCharacter->OnStrengthChanged.AddDynamic(this, &UAttributesWidget::SetStrengthPercent);
	PlayerCharacter->OnEnduranceChanged.AddDynamic(this, &UAttributesWidget::SetEndurancePercent);
	PlayerCharacter->OnVitalityChanged.AddDynamic(this, &UAttributesWidget::SetVitalityPercent);
	PlayerCharacter->OnAgilityChanged.AddDynamic(this, &UAttributesWidget::SetAgilityPercent);
	PlayerCharacter->OnDexterityChanged.AddDynamic(this, &UAttributesWidget::SetDexterityPercent);
	PlayerCharacter->OnPerceptionChanged.AddDynamic(this, &UAttributesWidget::SetPerceptionPercent);

	SetHealthPercent(PlayerCharacter->GetHealth(), PlayerCharacter->GetMaxHealth());
	SetStaminaPercent(PlayerCharacter->GetStamina(), PlayerCharacter->GetMaxStamina());
	SetStrengthPercent(PlayerCharacter->GetStrength(), PlayerCharacter->GetMaxStrength());
	SetEndurancePercent(PlayerCharacter->GetEndurance(), PlayerCharacter->GetMaxEndurance());
	SetVitalityPercent(PlayerCharacter->GetVitality(), PlayerCharacter->GetMaxVitality());
	SetAgilityPercent(PlayerCharacter->GetAgility(), PlayerCharacter->GetMaxAgility());
	SetDexterityPercent(PlayerCharacter->GetDexterity(), PlayerCharacter->GetMaxDexterity());
	SetPerceptionPercent(PlayerCharacter->GetPerception(), PlayerCharacter->GetMaxPerception());
}
