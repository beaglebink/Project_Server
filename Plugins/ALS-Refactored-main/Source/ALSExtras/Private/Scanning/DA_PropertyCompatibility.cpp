#include "Scanning/DA_PropertyCompatibility.h"

bool UDA_PropertyCompatibility::GetRule(const FGameplayTag& PropertyTag, FPropertyCompatibilityRule& OutRule) const
{
	for (const FPropertyCompatibilityRule& Rule : CompatibilityRules)
	{
		if (Rule.PropertyTag == PropertyTag)
		{
			OutRule = Rule;
			return true;
		}
	}

	return false;
}
