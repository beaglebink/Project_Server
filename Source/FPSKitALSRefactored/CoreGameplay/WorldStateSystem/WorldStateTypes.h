#pragma once

#include "CoreMinimal.h"
#include "WorldStateActorRef.h"
#include "WorldStateTypes.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// EWorldStateChangeCategory
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EWorldStateChangeCategory : uint8
{
    Structural          UMETA(DisplayName = "Structural"),
    InteractiveObject   UMETA(DisplayName = "Interactive Object"),
    Environment         UMETA(DisplayName = "Environment"),
    ActorState          UMETA(DisplayName = "Actor State"),
    Custom              UMETA(DisplayName = "Custom")
};

// ─────────────────────────────────────────────────────────────────────────────
// FWorldStateRecord
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FWorldStateRecord
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
    FName FactId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
    EWorldStateChangeCategory Category = EWorldStateChangeCategory::Custom;

    // ---- Where to apply ----
    // ---- Куда применять ----
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState",
        meta = (SceneActorFilter = "FloorAssignmentOnly"))
    FWorldStateActorRef ActorRef;

    // NAME_None → свойство ищется на самом актёре.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
    FName ComponentName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
    FName ChangeKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
    FString SerializedValue;

    FString OriginalValue;

    bool bHasOriginalValue = false;

    bool bPendingRemoval = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
    FName ReactionFunctionName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
    FString Timestamp;

    FWorldStateRecord() = default;

    FWorldStateRecord(
        FName InFactId,
        const FString& InDescription,
        EWorldStateChangeCategory InCategory,
        const FGuid& InItemId,
        FName InChangeKey,
        const FString& InValue,
        FName InComponentName = NAME_None,
        FName InReactionFunctionName = NAME_None)
        : FactId(InFactId)
        , Description(InDescription)
        , Category(InCategory)
        , ComponentName(InComponentName)   // порядок инициализации = порядок объявления
        , ChangeKey(InChangeKey)
        , SerializedValue(InValue)
        , ReactionFunctionName(InReactionFunctionName)
        , Timestamp(FDateTime::UtcNow().ToString())
    {
        ActorRef.ActorPersistentId = InItemId;
    }

    FORCEINLINE FGuid GetItemId() const { return ActorRef.ActorPersistentId; }
};