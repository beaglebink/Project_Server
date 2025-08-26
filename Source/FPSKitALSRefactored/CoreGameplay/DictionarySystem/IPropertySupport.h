#pragma once
#include "DictionaryObjectBase.h"
#include "IPropertySupport.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPropertySupport : public UInterface
{
    GENERATED_BODY()
};

class FPSKITALSREFACTORED_API IPropertySupport
{
    GENERATED_BODY()

public:
    virtual void ApplyProperty(const FString& PropertyName, const FVariantProperty& Value) = 0;
};
