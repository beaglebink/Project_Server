#include "StreetTransitionTrigger.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "LocationStreetTransitionPayload.h"
#include "LocationTrackerSubsystem.h"
#include "StreetAsset.h"
#include "Outcome.h"

AStreetTransitionTrigger::AStreetTransitionTrigger()
{
    PrimaryActorTick.bCanEverTick = false;

    TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
    RootComponent = TriggerVolume;
    TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    TriggerVolume->SetGenerateOverlapEvents(true);
}

void AStreetTransitionTrigger::NotifyActorBeginOverlap(AActor* OtherActor)
{
    Super::NotifyActorBeginOverlap(OtherActor);

    if (!OtherActor || !OtherActor->IsA<APawn>()) return;
    if (!TargetStreet) return;

    // Отменяем отложенную очистку — игрок вернулся (или вошёл впервые).
    if (ClearCheckTimer.IsValid())
    {
        GetWorldTimerManager().ClearTimer(ClearCheckTimer);
    }

    PublishStreetEnter();
}

void AStreetTransitionTrigger::NotifyActorEndOverlap(AActor* OtherActor)
{
    Super::NotifyActorEndOverlap(OtherActor);

    if (!OtherActor || !OtherActor->IsA<APawn>()) return;
    if (!TargetStreet) return;
    if (!bClearStreetOnExit) return;

    ScheduleClearCheck();
}

void AStreetTransitionTrigger::ScheduleClearCheck()
{
    if (ClearCheckTimer.IsValid())
    {
        GetWorldTimerManager().ClearTimer(ClearCheckTimer);
    }

    FTimerDelegate Del = FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            if (!IsValid(this)) return;
            if (!TargetStreet) return;
            if (!TriggerVolume) return;

            // 1) В триггере всё ещё кто-то есть — игрок вернулся.
            TArray<AActor*> Overlapping;
            TriggerVolume->GetOverlappingActors(Overlapping, APawn::StaticClass());
            if (Overlapping.Num() > 0) return;

            UGameInstance* GI = GetGameInstance();
            if (!GI) return;

            ULocationTrackerSubsystem* Tracker = GI->GetSubsystem<ULocationTrackerSubsystem>();
            if (!Tracker) return;

            const FLocationVisitAddress& Cur = Tracker->GetCurrentAddress();

            // 2) Игрок внутри дома/этажа — улицей управляет иерархия
            //    FloorAsset → InteriorSetAsset → StreetAsset, а не триггеры.
            if (Cur.BuildingId.IsValid() || Cur.FloorId.IsValid()) return;

            // 3) Текущая улица уже не наша — игрок вошёл в другой триггер
            //    (или трекер переключился по какой-то другой причине).
            if (Cur.StreetId != TargetStreet->StreetID) return;

            PublishStreetClear();
        });

    GetWorldTimerManager().SetTimer(
        ClearCheckTimer, Del, FMath::Max(ClearStreetDelay, 0.0f), /*bLoop=*/false);
}

void AStreetTransitionTrigger::PublishStreetEnter()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) return;

    UEventBusSubsystem* EventBus = GI->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    ULocationStreetTransitionPayload* Payload =
        EventBus->CreatePayload<ULocationStreetTransitionPayload>();
    if (!Payload) return;

    Payload->Setup(TargetStreet, /*bInClearStreet=*/false);

    FOutcomeEventBase Event;
    Event.OutcomeType = EOutcomeType::Interior;
    Event.OutcomeInterior = EOutcomeInterior::StreetTransition;
    Event.Payload = Payload;
    EventBus->PublishOutcome(Event);
}

void AStreetTransitionTrigger::PublishStreetClear()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) return;

    UEventBusSubsystem* EventBus = GI->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    ULocationStreetTransitionPayload* Payload =
        EventBus->CreatePayload<ULocationStreetTransitionPayload>();
    if (!Payload) return;

    Payload->Setup(TargetStreet, /*bInClearStreet=*/true);

    FOutcomeEventBase Event;
    Event.OutcomeType = EOutcomeType::Interior;
    Event.OutcomeInterior = EOutcomeInterior::StreetTransition;
    Event.Payload = Payload;
    EventBus->PublishOutcome(Event);
}