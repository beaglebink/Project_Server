#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class UMissionConditionAsset;

class FMissionConditionAssetDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    // IDetailCustomization interface
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};