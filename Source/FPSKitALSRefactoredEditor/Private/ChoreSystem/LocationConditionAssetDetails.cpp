#include "ChoreSystem/LocationConditionAssetDetails.h"
#include "DetailLayoutBuilder.h"
#include "CoreGameplay/ChoreSystem/LocationConditionAsset.h"

TSharedRef<IDetailCustomization> FLocationConditionAssetDetails::MakeInstance()
{
    return MakeShareable(new FLocationConditionAssetDetails);
}

void FLocationConditionAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    DetailBuilder.HideCategory("1 - Operator");
    DetailBuilder.HideCategory("2 - Composite Conditions");
    DetailBuilder.HideCategory("2 - Logic Operands");
    DetailBuilder.HideCategory("3 - Simple Condition");
}