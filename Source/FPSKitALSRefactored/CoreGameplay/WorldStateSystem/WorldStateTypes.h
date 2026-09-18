#pragma once

#include "CoreMinimal.h"
#include "WorldStateTypes.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// EWorldStateChangeCategory — семантическая категория постоянного изменения мира.
// Используется для группировки и фильтрации при восстановлении.
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EWorldStateChangeCategory : uint8
{
    // Структурное изменение (дверь сломана, стена пробита)
    Structural          UMETA(DisplayName = "Structural"),
    // Состояние интерактивного объекта (терминал взломан, замок открыт)
    InteractiveObject   UMETA(DisplayName = "Interactive Object"),
    // Состояние среды (мусор убран, ремонт сделан)
    Environment         UMETA(DisplayName = "Environment"),
    // Состояние персонажа/NPC (убит, союзник)
    ActorState          UMETA(DisplayName = "Actor State"),
    // Произвольный флаг (scripted)
    Custom              UMETA(DisplayName = "Custom")
};

// ─────────────────────────────────────────────────────────────────────────────
// FWorldStateKey — уникальный ключ записи.
// Тройка (ItemId, ComponentName, ChangeKey) позволяет хранить одноимённые
// свойства на разных компонентах одного и того же актёра без коллизий.
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FWorldStateKey
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FGuid ItemId;

    // NAME_None → свойство относится к самому актёру
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
// FWorldStateRecord — одна запись о постоянном изменении мира.
// Хранится в WorldStateSubsystem и сохраняется на диск через GameSaveSubsystem.
//
// Идентифицируется по ItemId (FGuid из UFloorAssignmentComponent) и, опционально,
// по ComponentName. SerializedValue — произвольная строка (ExportText свойств).
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FWorldStateRecord
{
    GENERATED_BODY()

    // Стабильный идентификатор объекта
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FGuid ItemId;

    // Категория изменения
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    EWorldStateChangeCategory Category = EWorldStateChangeCategory::Custom;

    // Ключ свойства или тег изменения (например "DoorOpen", "TerminalHacked").
    // Должен совпадать с именем FProperty, помеченного CPF_SaveGame.
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FName ChangeKey;

    // Имя компонента, в котором менять свойство.
    // NAME_None → свойство ищется на самом актёре.
    // Если задано — поиск идёт ТОЛЬКО в этом компоненте (fallback на актёр не выполняется).
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FName ComponentName = NAME_None;

    // Сериализованное значение (ExportText, "true"/"false", JSON-фрагмент)
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FString SerializedValue;

    // Время изменения (UTC, строка для простой сериализации)
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