#include "StreetTransitionTrigger.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameInstance.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "LocationStreetTransitionPayload.h"
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

    // Реагируем только на пешку игрока (или любого Pawn — уточните фильтр
    // под свой проект, если у вас есть NPC на тех же улицах).
    if (!OtherActor || !OtherActor->IsA<APawn>()) return;
    if (!TargetStreet) return;

    UGameInstance* GI = GetGameInstance();
    if (!GI) return;

    UEventBusSubsystem* EventBus = GI->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    ULocationStreetTransitionPayload* Payload =
        EventBus->CreatePayload<ULocationStreetTransitionPayload>();
    if (!Payload) return;

    Payload->Setup(TargetStreet);

    FOutcomeEventBase Event;
    Event.OutcomeType = EOutcomeType::Interior;
    Event.OutcomeInterior = EOutcomeInterior::StreetTransition;
    Event.Payload = Payload;
    EventBus->PublishOutcome(Event);
}