#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class UMissionActiveConditionAsset;

class FMissionActiveConditionAssetDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    // IDetailCustomization interface
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};