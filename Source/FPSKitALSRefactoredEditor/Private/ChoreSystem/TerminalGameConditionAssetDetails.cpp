#include "ChoreSystem/TerminalGameConditionAssetDetails.h"
#include "DetailLayoutBuilder.h"
#include "CoreGameplay/ChoreSystem/TerminalGameConditionAsset.h"

TSharedRef<IDetailCustomization> FTerminalGameConditionAssetDetails::MakeInstance()
{
    return MakeShareable(new FTerminalGameConditionAssetDetails);
}

void FTerminalGameConditionAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    // Скрываем базовые категории UOutcomeConditionAsset.
    DetailBuilder.HideCategory("1 - Operator");
    DetailBuilder.HideCategory("2 - Composite Conditions");
    DetailBuilder.HideCategory("2 - Logic Operands");
    DetailBuilder.HideCategory("3 - Simple Condition");

    // Остаются "Terminal Game" и "4 - Debug".
}