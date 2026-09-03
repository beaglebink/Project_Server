#include "ChoreSystem/MissionActiveConditionAssetDetails.h"
#include "DetailLayoutBuilder.h"
#include "CoreGameplay/ChoreSystem/MissionActiveConditionAsset.h"

TSharedRef<IDetailCustomization> FMissionActiveConditionAssetDetails::MakeInstance()
{
    return MakeShareable(new FMissionActiveConditionAssetDetails);
}

void FMissionActiveConditionAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    // Скрываем неиспользуемые категории
    DetailBuilder.HideCategory("1 - Operator");
    DetailBuilder.HideCategory("2 - Composite Conditions");
    DetailBuilder.HideCategory("2 - Logic Operands");
    DetailBuilder.HideCategory("3 - Simple Condition");

    // Категории "Mission" и "4 - Debug" останутся видимыми
}