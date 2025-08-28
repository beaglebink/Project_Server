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
    EPropertyValueType Type;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString VariableTypeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ValueName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool BoolValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 IntValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FloatValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString StringValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector VectorValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FLinearColor ColorValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AActor> ObjectValue;

    FString ToString() const;
};