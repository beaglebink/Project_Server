#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class FTerminalGameConditionAssetDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};