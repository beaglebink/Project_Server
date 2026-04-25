#pragma once

#include "CoreMinimal.h"
#include "WorldStateTypes.generated.h"

// ??? EWorldStateChangeCategory ????????????????????????????????????????????????
// Семантическая категория постоянного изменения мира.
// Используется для группировки и фильтрации при восстановлении.
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

// ??? FWorldStateRecord ????????????????????????????????????????????????????????
// Одна запись о постоянном изменении мира.
// Хранится в WorldStateSubsystem и сохраняется на диск через GameSaveSubsystem.
//
// Идентифицируется по ItemId (FGuid из UFloorAssignmentComponent).
// SerializedValue — произвольная строка (ExportText свойств, JSON, флаг).
// Источник изменения — MissionId (FName) для дебага и трассировки.
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FWorldStateRecord
{
    GENERATED_BODY()

    // Стабильный идентификатор объекта (совпадает с UFloorAssignmentComponent::ItemId)
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FGuid ItemId;

    // Категория изменения
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    EWorldStateChangeCategory Category = EWorldStateChangeCategory::Custom;

    // Ключ свойства или тег изменения (например "DoorOpen", "TerminalHacked")
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FName ChangeKey;

    // Сериализованное значение (ExportText, "true"/"false", JSON-фрагмент)
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FString SerializedValue;

    // Миссия, которая породила это изменение (для дебага)
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FName SourceMissionId;

    // Время изменения (UTC, строка для простой сериализации)
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FString Timestamp;

    FWorldStateRecord() = default;

    FWorldStateRecord(
        const FGuid& InItemId,
        EWorldStateChangeCategory InCategory,
        FName InChangeKey,
        const FString& InValue,
        FName InMissionId = NAME_None)
        : ItemId(InItemId)
        , Category(InCategory)
        , ChangeKey(InChangeKey)
        , SerializedValue(InValue)
        , SourceMissionId(InMissionId)
        , Timestamp(FDateTime::UtcNow().ToString())
    {}
};
