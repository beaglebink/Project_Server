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
// FWorldStateKey
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FWorldStateKey
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FGuid ItemId;

    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FName ComponentName = NAME_None;

    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FName ChangeKey;

    FWorldStateKey() = default;

    FWorldStateKey(const FGuid& InItemId, FName InComponentName, FName InChangeKey)
        : ItemId(InItemId), ComponentName(InComponentName), ChangeKey(InChangeKey) {
    }

    bool operator==(const FWorldStateKey& Other) const
    {
        return ItemId == Other.ItemId
            && ComponentName == Other.ComponentName
            && ChangeKey == Other.ChangeKey;
    }
};

FORCEINLINE uint32 GetTypeHash(const FWorldStateKey& Key)
{
    uint32 H = GetTypeHash(Key.ItemId);
    H = HashCombine(H, GetTypeHash(Key.ComponentName));
    H = HashCombine(H, GetTypeHash(Key.ChangeKey));
    return H;
}

// ─────────────────────────────────────────────────────────────────────────────
// FWorldStateRecord
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FWorldStateRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FGuid ItemId;

    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    EWorldStateChangeCategory Category = EWorldStateChangeCategory::Custom;

    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FName ChangeKey;

    // NAME_None → свойство ищется на самом актёре.
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FName ComponentName = NAME_None;

    // Значение, которое было установлено записью.
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FString SerializedValue;

    // Значение, которое было у свойства ДО установки записи.
    // Восстанавливается при удалении.
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FString OriginalValue;

    // true, если OriginalValue успешно захвачен.
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    bool bHasOriginalValue = false;

    // true → запись помечена на удаление, но ещё не финализирована
    // (актёр отсутствует или оригинал не захвачен).
    // Такие записи игнорируются функциями чтения и применяются только
    // для финализации удаления, когда актёр появится.
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    bool bPendingRemoval = false;

    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FString Timestamp;

    FWorldStateRecord() = default;

    FWorldStateRecord(
        const FGuid& InItemId,
        EWorldStateChangeCategory InCategory,
        FName InChangeKey,
        const FString& InValue,
        FName InComponentName = NAME_None)
        : ItemId(InItemId)
        , Category(InCategory)
        , ChangeKey(InChangeKey)
        , ComponentName(InComponentName)
        , SerializedValue(InValue)
        , Timestamp(FDateTime::UtcNow().ToString())
    {
    }
};