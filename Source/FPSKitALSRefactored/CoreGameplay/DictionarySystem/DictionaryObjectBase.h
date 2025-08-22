#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "DictionaryObjectBase.generated.h"

class ADictionaryManager;

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


UCLASS()
class FPSKITALSREFACTORED_API ADictionaryObjectBase : public AActor
{
    GENERATED_BODY()

public:


protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	FName KeyActorName = NAME_None;

protected:
    static ADictionaryManager* ManagerInstance;
};

USTRUCT(BlueprintType)
struct FVariantProperty
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPropertyValueType Type;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName VariableTypeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ValueName;

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
    ADictionaryObjectBase* ObjectValue;

    FString ToString() const
    {
        switch (Type)
        {
        case EPropertyValueType::Bool:   return BoolValue ? TEXT("true") : TEXT("false");
        case EPropertyValueType::Int:    return FString::FromInt(IntValue);
        case EPropertyValueType::Float:  return FString::SanitizeFloat(FloatValue);
        case EPropertyValueType::String: return StringValue;
        case EPropertyValueType::Vector: return VectorValue.ToString();
        case EPropertyValueType::Color:  return ColorValue.ToString();
        case EPropertyValueType::Object: return ObjectValue ? ObjectValue->GetName() : TEXT("None");
        default: return TEXT("Unknown");
        }
    }
};

USTRUCT(BlueprintType)
struct FDictionaryActorStruct : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ActorID;

    //UPROPERTY(EditAnywhere, BlueprintReadWrite)
    //FName TypeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName PropertyName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVariantProperty PropertyValue;
};