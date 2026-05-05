#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

class FProxyActorNamePinFactory : public FGraphPanelPinFactory
{
public:
    virtual TSharedPtr<SGraphPin> CreatePin(UEdGraphPin* Pin) const override;
};
