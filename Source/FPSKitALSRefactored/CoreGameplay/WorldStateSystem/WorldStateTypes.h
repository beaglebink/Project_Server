#pragma once

#include "CoreMinimal.h"
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
// Одна запись о постоянном изменении мира.
// Уникальный ключ — FactId (строковый идентификатор мирового факта).
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FWorldStateRecord
{
    GENERATED_BODY()

    // Строковый уникальный идентификатор факта. Например "DoorOpened_MainEntrance".
    // Служит ключом карты WorldStateRecords и используется для удаления записи.
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FName FactId;

    // Человекочитаемое описание факта (для UI, дебага, логов).
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FString Description;

    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    EWorldStateChangeCategory Category = EWorldStateChangeCategory::Custom;

    // ---- Куда применять ----
    // Идентификатор объекта (FGuid из UFloorAssignmentComponent).
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FGuid ItemId;

    // NAME_None → свойство ищется на самом актёре.
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FName ComponentName = NAME_None;

    // Имя FProperty, помеченного CPF_SaveGame.
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FName ChangeKey;

    // ---- Значения ----
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FString SerializedValue;

    FString OriginalValue;

    bool bHasOriginalValue = false;

    bool bPendingRemoval = false;

    // Имя UFUNCTION без параметров, вызываемой на целевом объекте после
    // применения значения. NAME_None — реакция не вызывается.
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FName ReactionFunctionName = NAME_None;

    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
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
        , ItemId(InItemId)
        , ComponentName(InComponentName)
        , ChangeKey(InChangeKey)
        , SerializedValue(InValue)
        , ReactionFunctionName(InReactionFunctionName)
        , Timestamp(FDateTime::UtcNow().ToString())
    {
    }
};
