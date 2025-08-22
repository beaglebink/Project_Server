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
    // ѕрименить свойство по имени с новым значением
    virtual void ApplyProperty(const FName PropertyName, const FVariantProperty Value) = 0;
	virtual void AddPropertyDescription(const FName PropertyName, const FName ValueName) = 0;
};
