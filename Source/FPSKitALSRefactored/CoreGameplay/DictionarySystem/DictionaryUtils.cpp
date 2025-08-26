#include "DictionaryUtils.h"

FString UDictionaryUtils::GetPropertyTypeName(EPropertyValueType Type)
{
    static const TMap<EPropertyValueType, FString> TypeNames = {
        { EPropertyValueType::Bool,   TEXT("Boolean") },
        { EPropertyValueType::Int,    TEXT("Integer") },
        { EPropertyValueType::Float,  TEXT("Float") },
        { EPropertyValueType::String, TEXT("Text") },
        { EPropertyValueType::Vector, TEXT("Vector3") },
        { EPropertyValueType::Color,  TEXT("Color") }
    };

    const FString* Found = TypeNames.Find(Type);
    return Found ? *Found : TEXT("Unknown");
}

FString UDictionaryUtils::GetValueText(const FVariantProperty& Property)
{
	return GetPropertyTypeName(Property.Type) + TEXT(": ") + Property.ValueName;
}