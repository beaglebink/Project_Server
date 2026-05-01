#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "../InteriorInstanceSystem/FloorPopulationTypes.h"
#include "MissionEnvelopeTypes.generated.h"

// Каналы объектов на этаже 
// Каждый канал соответствует семантической группе объектов.
// Если канал не объявлен в Envelope — он не затрагивается.
UENUM(BlueprintType)
enum class EEnvelopeChannel : uint8
{
    Clutter             UMETA(DisplayName = "Clutter / Loose Items"),
    InteriorObjects     UMETA(DisplayName = "Interior Objects"),
    DoorLocks           UMETA(DisplayName = "Door Locks"),
    SpawnGroups         UMETA(DisplayName = "Spawn Groups"),
    TerminalState       UMETA(DisplayName = "Terminal State"),
    ActorPlacement      UMETA(DisplayName = "Actor Placement"),
    ActorAvailability   UMETA(DisplayName = "Actor Availability"),
    DialogueAccess      UMETA(DisplayName = "Dialogue Access"),
    InventoryItems      UMETA(DisplayName = "Inventory Items"),
    LocationTriggers    UMETA(DisplayName = "Location Triggers")
};

// Политики поведения канала при завершении миссии 
UENUM(BlueprintType)
enum class EChannelPolicy : uint8
{
    // Сбросить объекты к исходному состоянию
    Reset               UMETA(DisplayName = "Reset"),
    // Заморозить — не изменять состояние при входе/выходе
    Freeze              UMETA(DisplayName = "Freeze"),
    // Сохранить только идентификатор (для StableActors)
    PersistIdentityOnly UMETA(DisplayName = "Persist Identity Only"),
    // Сброс если не зачищено (для SpawnGroups)
    ResetUnlessCleared  UMETA(DisplayName = "Reset Unless Cleared"),
    // Поведение определяется скриптом миссии
    MissionScripted     UMETA(DisplayName = "Mission Scripted")
};

// Политика пространства задания 
UENUM(BlueprintType)
enum class EJobSpacePolicy : uint8
{
    // Не применять никаких правил — поведение мира по умолчанию
    None    UMETA(Hidden, DisplayName = "None"),
    // Все изменения сбрасываются при уходе из здания
    Reset   UMETA(DisplayName = "Reset All"),
    // Все изменения заморожены
    Freeze  UMETA(DisplayName = "Freeze All"),
    // Каналы определяются явно
    Partial UMETA(DisplayName = "Partial (Per Channel)")
};

// Режим возобновления при загрузке 
UENUM(BlueprintType)
enum class EMissionResumeMode : uint8
{
    // Миссия продолжается после загрузки (по умолчанию)
    Resumable       UMETA(DisplayName = "Resumable"),
    // Миссия перезапускается при загрузке
    RestartOnLoad   UMETA(DisplayName = "Restart On Load"),
    // Миссия помечается как проваленная при загрузке
    FailOnLoad      UMETA(DisplayName = "Fail On Load")
};

// Причина завершения миссии 
UENUM(BlueprintType)
enum class EMissionEndReason : uint8
{
    None        UMETA(Hidden, DisplayName = "None"),
    Completed   UMETA(DisplayName = "Completed"),
    Failed      UMETA(DisplayName = "Failed"),
    Abandoned   UMETA(DisplayName = "Abandoned")
};

// Политика выхода 
// Определяет что происходит при уходе с этажа и из здания.
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FMissionExitPolicy
{
    GENERATED_BODY()

    // Переход между этажами внутри здания — Envelope не освобождается.
    // (Это поведение фиксировано, поле только для документации.)
    //UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ExitPolicy")
    //bool bFloorTransitionPreservesEnvelope = true;

    // Что делать с пространством задания при уходе из здания (во время активной миссии)
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExitPolicy")
    //EJobSpacePolicy OnLeaveBuilding = EJobSpacePolicy::None;

    // Что делать с постоянным хранилищем при успешном завершении миссии (MissionCompleted)
    // ResetAll    — не обновлять FloorStateSnapshots
    // FreezeAll   — сохранить все акторы в FloorStateSnapshots
    // Partial     — сохранить по каналам согласно Channels

};

// Описание канала с политикой 
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FEnvelopeChannelEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channel")
    EEnvelopeChannel Channel = EEnvelopeChannel::Clutter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Channel")
    EChannelPolicy Policy = EChannelPolicy::Reset;
};

// Scope — зона действия Envelope 
// Здание идентифицируется по DisplayName (UInteriorSetAsset::DisplayName).
// Этажи — явным списком soft-ссылок на FloorAsset.
// Этажи вне списка — не затрагиваются.
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FMissionEnvelopeScope
{
    GENERATED_BODY()

    // Отображаемое имя здания (должно совпадать с UInteriorSetAsset::DisplayName)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
    FText BuildingDisplayName;

    // Явный список этажей в области действия Envelope
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scope")
    TArray<TSoftObjectPtr<class UFloorAsset>> InteriorScopes;
};

// Envelope — весь блок настроек persistence 
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FMissionEnvelope
{
    GENERATED_BODY()

    // Зона действия
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Envelope|Scope")
    FMissionEnvelopeScope Scope;

    // Политика JobSpacePolicy.
    // Если Partial — каждый канал в Channels должен быть объявлен явно.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Envelope|Policy")
    EJobSpacePolicy JobSpacePolicy = EJobSpacePolicy::Freeze;

    // Каналы с политиками. Используется только при JobSpacePolicy = Partial.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Envelope|Channels",
        meta = (EditCondition = "JobSpacePolicy == EJobSpacePolicy::Partial", EditConditionHides))
    TArray<FEnvelopeChannelEntry> Channels;

    // Политика выхода
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExitPolicy", meta = (DisplayName = "Stage Complete Policy"))
    EJobSpacePolicy OnMissionCompleted = EJobSpacePolicy::Freeze;

    // Каналы с политиками выхода. Используется только при ExitPolicy = Partial.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ExitPolicy",
        meta = (DisplayName = "Stage Exit Channels", EditCondition = "OnMissionCompleted == EJobSpacePolicy::Partial", EditConditionHides))
    TArray<FEnvelopeChannelEntry> ExitChannels;

    // Приоритет — при конфликте каналов побеждает envelope с меньшим числом
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Envelope|Priority")
    //int32 Priority = 0;

    // Режим возобновления после загрузки сохранения
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Envelope|Save")
    EMissionResumeMode ResumeMode = EMissionResumeMode::Resumable;

    // Возвращает политику для указанного канала. Если канал не объявлен — TOptional пуст.
    TOptional<EChannelPolicy> GetPolicyForChannel(EEnvelopeChannel InChannel) const
    {
        for (const FEnvelopeChannelEntry& Entry : Channels)
        {
            if (Entry.Channel == InChannel)
            {
                return Entry.Policy;
            }
        }
        return TOptional<EChannelPolicy>();
    }

	// Проверяет что Envelope имеет смысл (не пустой scope)
    bool IsValid() const
    {
        return !Scope.InteriorScopes.IsEmpty() || !Scope.BuildingDisplayName.IsEmpty();
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Маппинг EFloorActorType → EEnvelopeChannel
// Используется InteriorSubsystem при решении применять политику канала
// к конкретному актору в зависимости от его семантического типа.
// ─────────────────────────────────────────────────────────────────────────────
FORCEINLINE EEnvelopeChannel FloorActorTypeToEnvelopeChannel(EFloorActorType ActorType)
{
    switch (ActorType)
    {
    case EFloorActorType::LightItem:
    case EFloorActorType::HeavyFurniture:
        return EEnvelopeChannel::InteriorObjects;

    case EFloorActorType::Debris:
        return EEnvelopeChannel::Clutter;

	case EFloorActorType::DoorLocks:
		return EEnvelopeChannel::DoorLocks;

    case EFloorActorType::StableActor:
        return EEnvelopeChannel::ActorPlacement;

	case EFloorActorType::DialogueAccess:
		return EEnvelopeChannel::DialogueAccess;

    case EFloorActorType::Terminal:
        return EEnvelopeChannel::TerminalState;

    case EFloorActorType::NPC_Spawner:
        return EEnvelopeChannel::SpawnGroups;

    case EFloorActorType::InventoryItems:
		return EEnvelopeChannel::InventoryItems;

    case EFloorActorType::LocationTriggers:
		return EEnvelopeChannel::LocationTriggers;

    default:
        return EEnvelopeChannel::Clutter;
    }
}

