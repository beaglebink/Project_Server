// CheckRequestPayload.h
#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "CheckRequestPayload.generated.h"

UENUM(BlueprintType)
enum class ECheckDataType : uint8
{
    Bool,
    Int32,
    Float,
    String
};

UENUM(BlueprintType)
enum class ECheckCompareOp : uint8
{
    Equal,
    NotEqual,
    Less,
    LessOrEqual,
    Greater,
    GreaterOrEqual
};

UENUM(BlueprintType)
enum class EMissionCheckProperty : uint8
{
    IsActive    UMETA(DisplayName = "Is Active"),
    CurrentStep UMETA(DisplayName = "Current Step"),
    Progress    UMETA(DisplayName = "Progress"),
    Time        UMETA(DisplayName = "Time"),
    Status      UMETA(DisplayName = "Status"),
    Name        UMETA(DisplayName = "Mission Name")
};

UCLASS(Abstract, BlueprintType)
class FPSKITALSREFACTORED_API UCheckRequestPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Check")
    FGuid TransactionId;

    UPROPERTY(BlueprintReadWrite, Category = "Check")
    ECheckDataType DataType = ECheckDataType::Bool;

    UPROPERTY(BlueprintReadWrite, Category = "Check")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    UPROPERTY(BlueprintReadWrite, Category = "Check")
    FString StringValue;

    UPROPERTY(BlueprintReadWrite, Category = "Check")
    FString ContextData;
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UMissionCheckRequestPayload : public UCheckRequestPayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Check")
    FName MissionId;

    UPROPERTY(BlueprintReadWrite, Category = "Check")
    EMissionCheckProperty PropertyToCheck = EMissionCheckProperty::IsActive;
};