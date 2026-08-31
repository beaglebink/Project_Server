// ChoreEnums.h
#pragma once

#include "CoreMinimal.h"
#include "ChoreEnums.generated.h"

UENUM(BlueprintType)
enum class EChoreFamily : uint8
{
    MovingObjects      UMETA(DisplayName = "Moving Objects"),
    SprayingCleaning   UMETA(DisplayName = "Spraying / Cleaning"),
    ScanningBagging    UMETA(DisplayName = "Scanning & Bagging"),
    Cooking            UMETA(DisplayName = "Cooking"),
    Delivery           UMETA(DisplayName = "Delivery"),
    Internet           UMETA(DisplayName = "Internet"),
    Miscellaneous      UMETA(DisplayName = "Miscellaneous")
};

UENUM(BlueprintType)
enum class EChoreSubtype : uint8
{
    Default            UMETA(DisplayName = "Default"),
    // Можно добавлять свои подтипы по мере необходимости
};

UENUM(BlueprintType)
enum class EChoreStatus : uint8
{
    Unavailable        UMETA(DisplayName = "Unavailable"),
    Available          UMETA(DisplayName = "Available"),
    Offered            UMETA(DisplayName = "Offered"),
    Accepted           UMETA(DisplayName = "Accepted"),
    WaitingToStart     UMETA(DisplayName = "Waiting to Start"),
    Active             UMETA(DisplayName = "Active"),
    Succeeded          UMETA(DisplayName = "Succeeded"),
    Failed             UMETA(DisplayName = "Failed"),
    Expired            UMETA(DisplayName = "Expired"),
    RetryAvailable     UMETA(DisplayName = "Retry Available")
};

UENUM(BlueprintType)
enum class EChoreRetryBehavior : uint8
{
    None               UMETA(DisplayName = "No Retry"),
    Immediate          UMETA(DisplayName = "Immediate Retry Available"),
    RequireReaccept    UMETA(DisplayName = "Require Re-accept from NPC"),
    Conditional        UMETA(DisplayName = "Conditional (Reactivation Condition)")
};

UENUM(BlueprintType)
enum class EChoreHistoryQueryType : uint8
{
    CountCompleted     UMETA(DisplayName = "Count Completed"),
    CountSucceeded     UMETA(DisplayName = "Count Succeeded"),
    BestPerformance    UMETA(DisplayName = "Best Performance (specific metric)"),
    LastResult         UMETA(DisplayName = "Last Result (Success/Fail)")
};