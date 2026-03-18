#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "FVariantProperty.generated.h"

class ADictionaryObjectBase;

UENUM(BlueprintType)
enum class EPropertyValueType : uint8
{
    Bool     UMETA(DisplayName = "Bool"),
    Int      UMETA(DisplayName = "Int"),
    Float    UMETA(DisplayName = "Float"),
    String   UMETA(DisplayName = "String"),
    Vector   UMETA(DisplayName = "Vector"),
    Color    UMETA(DisplayName = "Color"),
    Object   UMETA(DisplayName = "Object")
};

USTRUCT(BlueprintType)
struct FVariantProperty
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPropertyValueType Type = EPropertyValueType::Object;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString VariableTypeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ValueName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool BoolValue = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 IntValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FloatValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString StringValue = "";

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector VectorValue = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor ColorValue = FLinearColor(0, 0, 0, 0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AActor> ObjectValue;

    FString ToString() const;
};