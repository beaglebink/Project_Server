#include "ChoreSystem/MissionConditionAssetDetails.h"
#include "DetailLayoutBuilder.h"
#include "CoreGameplay/ChoreSystem/MissionConditionAsset.h"

TSharedRef<IDetailCustomization> FMissionConditionAssetDetails::MakeInstance()
{
    return MakeShareable(new FMissionConditionAssetDetails);
}

void FMissionConditionAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    // Скрываем неиспользуемые категории
    DetailBuilder.HideCategory("1 - Operator");
    DetailBuilder.HideCategory("2 - Composite Conditions");
    DetailBuilder.HideCategory("2 - Logic Operands");
    DetailBuilder.HideCategory("3 - Simple Condition");

    // Категории "Mission" и "4 - Debug" останутся видимыми
}