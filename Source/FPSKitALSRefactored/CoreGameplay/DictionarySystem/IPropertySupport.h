#pragma once

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
    virtual void ApplyProperty(const FName PropertyName, const FString& Value) = 0;
};
