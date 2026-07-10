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

    if (SnapshotChannel == ESnapshotChannel::Snapshot && ItemId.IsValid())
    {
        if (UWorld* W = GetWorld())
        {
            if (UGameInstance* GI = W->GetGameInstance())
            {
                if (UEventBusSubsystem* Bus = GI->GetSubsystem<UEventBusSubsystem>())
                {
                    UFloorPlacementPayload* Payload = Bus->CreatePayload<UFloorPlacementPayload>();
                    if (Payload)
                    {
                        AActor* Owner = GetOwner();
                        TArray<FName> TextTags;
                        if (Owner)
                        {
                            TextTags = Owner->Tags;
                        }
                        Payload->SetupWithTags(
                            ActorType,
                            Owner ? Owner->GetActorTransform() : FTransform::Identity,
                            ItemId,
                            Owner ? Owner->GetClass() : nullptr,
                            this->GameplayTagContainer,
                            TextTags
                            ,false);

                        IsRuntimeSpawned = false;
                        FOutcomeEventBase Ev;
                        Ev.OutcomeType = EOutcomeType::Interior;
                        Ev.OutcomeInterior = EOutcomeInterior::FloorPlacementRegistered;
                        Ev.Payload = Payload;
                        Bus->PublishOutcome(Ev);
                    }
                }
            }
        }
    }
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
                    AActor* Owner = GetOwner();
                    TArray<FName> TextTags;
                    if (Owner)
                    {
                        TextTags = Owner->Tags;
                    }
                    Payload->SetupWithTags(
                        ActorType,
                        Owner ? Owner->GetActorTransform() : FTransform::Identity,
                        ItemId,
                        Owner ? Owner->GetClass() : nullptr,
                        this->GameplayTagContainer,
                        TextTags,
                        IsRuntimeSpawned
                    );
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

void UFloorAssignmentComponent::PostEditImport()
{
    Super::PostEditImport();

    if (!HasAnyFlags(RF_ClassDefaultObject))
    {
        ItemId = FGuid::NewGuid();
        ProtectedItemId = ItemId;
        MarkPackageDirty();
    }
}

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

void UFloorAssignmentComponent::PublishRegistration()
{
    if (SnapshotChannel != ESnapshotChannel::Snapshot || !ItemId.IsValid())
        return;

    UWorld* W = GetWorld();
    if (!W) return;

    UGameInstance* GI = W->GetGameInstance();
    if (!GI) return;

    UEventBusSubsystem* Bus = GI->GetSubsystem<UEventBusSubsystem>();
    if (!Bus) return;

    UFloorPlacementPayload* Payload = Bus->CreatePayload<UFloorPlacementPayload>();
    if (!Payload) return;

    AActor* Owner = GetOwner();
    TArray<FName> TextTags;
    if (Owner)
        TextTags = Owner->Tags;

    Payload->SetupWithTags(
        ActorType,
        Owner ? Owner->GetActorTransform() : FTransform::Identity,
        ItemId,
        Owner ? Owner->GetClass() : nullptr,
        this->GameplayTagContainer,
        TextTags,
        true
    );

    IsRuntimeSpawned = true;
    FOutcomeEventBase Ev;
    Ev.OutcomeType = EOutcomeType::Interior;
    Ev.OutcomeInterior = EOutcomeInterior::FloorPlacementRegistered;
    Ev.Payload = Payload;
    Bus->PublishOutcome(Ev);
}

void UFloorAssignmentComponent::Registrate(EFloorActorType Type, FGuid ForceItemId /*= FGuid()*/)
{
    ItemId = ForceItemId.IsValid() ? ForceItemId : FGuid::NewGuid();
    ProtectedItemId = ItemId;
    SnapshotChannel = ESnapshotChannel::Snapshot;
    ActorType = Type;
    MarkPackageDirty();

    // Публикуем событие обновления регистрации (теги уже установлены в Blueprint)
    PublishRegistration();
}