#include "ChoreSystem/WorldStateConditionAssetDetails.h"
#include "DetailLayoutBuilder.h"
#include "CoreGameplay/ChoreSystem/WorldStateConditionAsset.h"

TSharedRef<IDetailCustomization> FWorldStateConditionAssetDetails::MakeInstance()
{
    return MakeShareable(new FWorldStateConditionAssetDetails);
}

void FWorldStateConditionAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    // Скрываем неиспользуемые категории базового UOutcomeConditionAsset.
    DetailBuilder.HideCategory("1 - Operator");
    DetailBuilder.HideCategory("2 - Composite Conditions");
    DetailBuilder.HideCategory("2 - Logic Operands");
    DetailBuilder.HideCategory("3 - Simple Condition");

    // Категории "WorldState" и "4 - Debug" останутся видимыми.
}