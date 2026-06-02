#pragma once

#include "CoreMinimal.h"
#include "SpawnGroupTypes.generated.h"

// Уникальный идентификатор спавн-группы.
// Для авторских групп дизайнер задаёт стабильный GUID.
// Для сгенерированных из пула он вычисляется детерминированно.
USTRUCT(BlueprintType)
struct FSpawnGroupId
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid Id;

    FSpawnGroupId() {}
    explicit FSpawnGroupId(const FGuid& InId) : Id(InId) {}

    bool IsValid() const { return Id.IsValid(); }
    bool operator==(const FSpawnGroupId& Other) const { return Id == Other.Id; }

    FString ToString() const { return Id.ToString(); }

    static FSpawnGroupId New() { return FSpawnGroupId(FGuid::NewGuid()); }
};

FORCEINLINE uint32 GetTypeHash(const FSpawnGroupId& Key)
{
    return GetTypeHash(Key.Id);
}

// Статус группы
UENUM(BlueprintType)
enum class ESpawnGroupStatus : uint8
{
    Inactive,           // Не применена к текущему интерьеру
    Active,             // Применена, спавны присутствуют
    PartiallyCleared,   // Часть целей выполнена
    Cleared,            // Полностью зачищена
    Suppressed          // Временно отключена (например, сюжетом)
};

// Причина зачистки (ResolutionReason)
UENUM(BlueprintType)
enum class ESpawnGroupResolutionReason : uint8
{
    None,
    Eliminated,     // Уничтожены все враги
    Captured,       // Захвачены (например, пленные)
    ScriptedRemoval,// Сюжетное удаление
    Other
};

// Группировка записей о спавнах в группе
USTRUCT(BlueprintType)
struct FSpawnGroupRecord
{
    GENERATED_BODY()

    // Актор, который был заспавнен (для отслеживания существования)
    UPROPERTY()
    TWeakObjectPtr<AActor> SpawnedActor;

    // Исходный класс/шаблон (для респавна)
    UPROPERTY()
    TSubclassOf<AActor> ActorClass;

    // Трансформ при спавне (для респавна)
    UPROPERTY()
    FTransform SpawnTransform;

    // Флаг, что этот элемент уже "очищен" (уничтожен/захвачен)
    UPROPERTY()
    bool bIsResolved = false;

    // Причина очистки этого конкретного элемента (опционально)
    UPROPERTY()
    ESpawnGroupResolutionReason ResolutionReason = ESpawnGroupResolutionReason::None;
};

// Полное состояние спавн-группы для одного этажа/здания
USTRUCT(BlueprintType)
struct FSpawnGroupState
{
    GENERATED_BODY()

    // Идентификатор группы
    UPROPERTY()
    FSpawnGroupId GroupId;

    // Статус группы
    UPROPERTY()
    ESpawnGroupStatus Status = ESpawnGroupStatus::Inactive;

    // Причина зачистки (если Status == Cleared)
    UPROPERTY()
    ESpawnGroupResolutionReason ResolutionReason = ESpawnGroupResolutionReason::None;

    // Последний визит / метка миссии (для политик)
    UPROPERTY()
    FName LastMissionContext;

    // Индекс посещения (например, 0,1,2...)
    UPROPERTY()
    int32 VisitIndex = 0;

    // Детальное состояние каждого элемента (опционально, только если требуется точное отслеживание)
    UPROPERTY()
    TArray<FSpawnGroupRecord> SpawnedRecords;
};