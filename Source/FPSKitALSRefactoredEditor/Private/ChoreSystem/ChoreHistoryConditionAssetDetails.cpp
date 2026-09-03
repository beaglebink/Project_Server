#include "ChoreSystem/ChoreHistoryConditionAssetDetails.h"
#include "DetailLayoutBuilder.h"
#include "CoreGameplay/ChoreSystem/ChoreHistoryConditionAsset.h"

TSharedRef<IDetailCustomization> FChoreHistoryConditionAssetDetails::MakeInstance()
{
    return MakeShareable(new FChoreHistoryConditionAssetDetails);
}

void FChoreHistoryConditionAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    DetailBuilder.HideCategory("1 - Operator");
    DetailBuilder.HideCategory("2 - Composite Conditions");
    DetailBuilder.HideCategory("2 - Logic Operands");
    DetailBuilder.HideCategory("3 - Simple Condition");
}