#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "DictionaryObjectBase.h"
#include "DictionaryUtils.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UDictionaryUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /* Returns the string name of the property type */
    UFUNCTION(BlueprintPure, Category = "Dictionary")
    static FString GetPropertyTypeName(EPropertyValueType Type);

    UFUNCTION(BlueprintPure, Category = "Dictionary")
    static FString GetValueText(const FVariantProperty& Property);
};
