#include "ChoreSystem/TimeWindowConditionAssetDetails.h"
#include "DetailLayoutBuilder.h"
#include "ConditionStateSubsystem/TimeWindowConditionAsset.h"

TSharedRef<IDetailCustomization> FTimeWindowConditionAssetDetails::MakeInstance()
{
    return MakeShareable(new FTimeWindowConditionAssetDetails);
}

void FTimeWindowConditionAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    DetailBuilder.HideCategory("1 - Operator");
    DetailBuilder.HideCategory("2 - Composite Conditions");
    DetailBuilder.HideCategory("2 - Logic Operands");
    DetailBuilder.HideCategory("3 - Simple Condition");
}