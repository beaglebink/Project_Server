#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"  // для FTransform, FGuid
#include "SpawnGroupTypes.generated.h"

// Уникальный идентификатор спавн-группы.
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
    Inactive,
    Active,
    PartiallyCleared,
    Cleared,
    Suppressed
};

// Причина зачистки
UENUM(BlueprintType)
enum class ESpawnGroupResolutionReason : uint8
{
    None,
    Eliminated,
    Captured,
    ScriptedRemoval,
    Other
};

// Запись о спавне (не используется в основном потоке, но оставим)
USTRUCT(BlueprintType)
struct FSpawnGroupRecord
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<AActor> SpawnedActor;

    UPROPERTY()
    TSubclassOf<AActor> ActorClass;

    UPROPERTY()
    FTransform SpawnTransform;

    UPROPERTY()
    bool bIsResolved = false;

    UPROPERTY()
    ESpawnGroupResolutionReason ResolutionReason = ESpawnGroupResolutionReason::None;
};

// Структура слота для полного сохранения
USTRUCT(BlueprintType)
struct FSpawnSlotState
{
    GENERATED_BODY()

    UPROPERTY()
    FGuid ItemId;

    UPROPERTY()
    TSubclassOf<AActor> ActorClass;

    UPROPERTY()
    FTransform SpawnTransform = FTransform::Identity;

    UPROPERTY()
    bool bIsAlive = true;

    // Можно расширить для хранения SaveGame-свойств
    // UPROPERTY()
    // FString ActorPropertiesJSON;
};

// Полное состояние спавн-группы
USTRUCT(BlueprintType)
struct FSpawnGroupState
{
    GENERATED_BODY()

    UPROPERTY()
    FGuid GroupId;

    UPROPERTY()
    ESpawnGroupStatus Status = ESpawnGroupStatus::Inactive;

    UPROPERTY()
    ESpawnGroupResolutionReason ResolutionReason = ESpawnGroupResolutionReason::None;

    UPROPERTY()
    FName LastMissionContext;

    UPROPERTY()
    int32 VisitIndex = 0;

    // Для режима по умолчанию (IsStoreSpawnParameters == false)
    UPROPERTY()
    TMap<FName, int32> TypeKilled;

    // Для режима полного сохранения (IsStoreSpawnParameters == true)
    UPROPERTY()
    TArray<FSpawnSlotState> Slots;

    // Сохранённое значение флага спавнера
    UPROPERTY()
    bool bStoreSpawnParameters = false;
};