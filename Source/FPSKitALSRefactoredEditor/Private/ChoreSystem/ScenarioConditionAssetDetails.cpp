#include "ChoreSystem/ScenarioConditionAssetDetails.h"
#include "DetailLayoutBuilder.h"
#include "CoreGameplay/ChoreSystem/ScenarioConditionAsset.h"

TSharedRef<IDetailCustomization> FScenarioConditionAssetDetails::MakeInstance()
{
    return MakeShareable(new FScenarioConditionAssetDetails);
}

void FScenarioConditionAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    DetailBuilder.HideCategory("1 - Operator");
    DetailBuilder.HideCategory("2 - Composite Conditions");
    DetailBuilder.HideCategory("2 - Logic Operands");
    DetailBuilder.HideCategory("3 - Simple Condition");
}