#include "ChoreSystem/ChoreTimeSinceConditionAssetDetails.h"
#include "DetailLayoutBuilder.h"
#include "CoreGameplay/ChoreSystem/ChoreTimeSinceConditionAsset.h"

TSharedRef<IDetailCustomization> FChoreTimeSinceConditionAssetDetails::MakeInstance()
{
    return MakeShareable(new FChoreTimeSinceConditionAssetDetails);
}

void FChoreTimeSinceConditionAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    DetailBuilder.HideCategory("1 - Operator");
    DetailBuilder.HideCategory("2 - Composite Conditions");
    DetailBuilder.HideCategory("2 - Logic Operands");
    DetailBuilder.HideCategory("3 - Simple Condition");
}