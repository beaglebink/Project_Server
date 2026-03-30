#include "MissionSubsystem.h"
#include "MissionProgressOutcome.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../EventBusSystem/OutcomeQuery.h"

void UMissionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        // Handler 1: GhostCleared AND (SpawnGroup != Default OR Actor != Default) AND (Object != Default OR Interior != Default) (Обработчик 1: GhostCleared И (спауна не Default ИЛИ актёра не Default) И (объект не Default ИЛИ интерьер не Default))
        auto GhostClearedQuery = FOutcomeQueryBuilder::And();
        GhostClearedQuery->Add(FOutcomeQueryBuilder::Type(EOutcomeType::GhostCleared));
        
        auto SpawnOrActor = FOutcomeQueryBuilder::Or();
        SpawnOrActor->Add(FOutcomeQueryBuilder::Not(FOutcomeQueryBuilder::SpawnGroup(EOutcomeSpawnGroup::Default)));
        SpawnOrActor->Add(FOutcomeQueryBuilder::Not(FOutcomeQueryBuilder::Actor(EOutcomeActor::Default)));
        GhostClearedQuery->Add(SpawnOrActor);
        
        auto ObjectOrInterior = FOutcomeQueryBuilder::Or();
        ObjectOrInterior->Add(FOutcomeQueryBuilder::Not(FOutcomeQueryBuilder::Object(EOutcomeObject::Default)));
        ObjectOrInterior->Add(FOutcomeQueryBuilder::Not(FOutcomeQueryBuilder::Interior(EOutcomeInterior::Default)));
        GhostClearedQuery->Add(ObjectOrInterior);

        EventBus->RegisterHandler(
            FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::OnGhostClearedWithConditions),
            GhostClearedQuery
        );

        // Handler 2: ItemAcquired AND Mission != Default (Обработчик 2: ItemAcquired И миссия не Default)
        auto ItemQuery = FOutcomeQueryBuilder::And();
        ItemQuery->Add(FOutcomeQueryBuilder::Type(EOutcomeType::ItemAcquired));
        ItemQuery->Add(FOutcomeQueryBuilder::Not(FOutcomeQueryBuilder::Mission(EOutcomeMission::Default)));

        EventBus->RegisterHandler(
            FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::OnItemAcquiredWithMission),
            ItemQuery
        );

        UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem: Registered 2 event handlers"));
    }
}

void UMissionSubsystem::OnGhostClearedWithConditions(const FOutcomeEventBase& Outcome)
{
    // Cast to specialized type (Кастируем в специализированный тип)
    const FMissionProgressOutcome* MissionProgress = static_cast<const FMissionProgressOutcome*>(&Outcome);
    
    if (MissionProgress)
    {
        // Handle mission-specific data when ghost is cleared (Обработка специфичных данных миссии при очищении привидения)
        UE_LOG(LogTemp, Log, 
            TEXT("MissionSubsystem: Mission %s advanced - Ghost cleared")
            TEXT(" | SpawnGroup: %d, Actor: %d, Object: %d, Interior: %d"),
            *MissionProgress->MissionName,
            static_cast<int32>(MissionProgress->OutcomeSpawnGroup),
            static_cast<int32>(MissionProgress->OutcomeActor),
            static_cast<int32>(MissionProgress->OutcomeObject),
            static_cast<int32>(MissionProgress->OutcomeInterior)
        );
    }
}

void UMissionSubsystem::OnItemAcquiredWithMission(const FOutcomeEventBase& Outcome)
{
    // Cast to specialized type (Кастируем в специализированный тип)
    const FMissionProgressOutcome* ItemProgress = static_cast<const FMissionProgressOutcome*>(&Outcome);
    
    if (ItemProgress)
    {
        // Handle mission-specific data when item is acquired (Обработка специфичных данных миссии при получении предмета)
        UE_LOG(LogTemp, Log,
            TEXT("MissionSubsystem: Item acquired - mission %s progress tracked")
            TEXT(" | Mission: %d"),
            *ItemProgress->MissionName,
            static_cast<int32>(ItemProgress->OutcomeMission)
        );
    }
}
