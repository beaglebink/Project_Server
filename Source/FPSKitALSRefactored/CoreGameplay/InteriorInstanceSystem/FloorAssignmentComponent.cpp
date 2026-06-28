#include "FloorAssignmentComponent.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "FloorPlacementPayload.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UFloorAssignmentComponent::UFloorAssignmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UFloorAssignmentComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UFloorAssignmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (SnapshotChannel == ESnapshotChannel::None || !ItemId.IsValid())
        return;

    if (EndPlayReason != EEndPlayReason::Destroyed)
        return;

    if (UWorld* W = GetWorld())
    {
        if (UGameInstance* GI = W->GetGameInstance())
        {
            if (UEventBusSubsystem* Bus = GI->GetSubsystem<UEventBusSubsystem>())
            {
                UFloorPlacementPayload* Payload = Bus->CreatePayload<UFloorPlacementPayload>();
                if (Payload)
                {
                    Payload->Setup(ActorType, GetOwner()->GetActorTransform(), ItemId);
                    FOutcomeEventBase Ev;
                    Ev.OutcomeType = EOutcomeType::Interior;
                    Ev.OutcomeInterior = EOutcomeInterior::FloorPlacementUnregistered;
                    Ev.Payload = Payload;
                    Bus->PublishOutcome(Ev);
                }
            }
        }
    }

    Super::EndPlay(EndPlayReason);
}

// ----- Обработка дублирования (Ctrl+D, Copy-Paste) -----
void UFloorAssignmentComponent::PostEditImport()
{
    Super::PostEditImport();

    // Генерируем новый ID только для реальных экземпляров (не CDO)
    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        ItemId = FGuid::NewGuid();
        ProtectedItemId = ItemId;
        MarkPackageDirty();
    }
}

// ----- Пометка пакета грязным для сохранения -----
void UFloorAssignmentComponent::MarkPackageDirty()
{
#if WITH_EDITOR
    Modify();
    if (UPackage* Package = GetOutermost())
        Package->MarkPackageDirty();
    if (AActor* Owner = GetOwner())
        Owner->MarkPackageDirty();
#endif
}