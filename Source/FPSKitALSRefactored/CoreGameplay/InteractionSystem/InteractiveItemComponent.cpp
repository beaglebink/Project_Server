#include "InteractiveItemComponent.h"
#include "InteractivePickerComponent.h"
#include "InteractItemStatePayload.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "../TerminalSystem/TerminalSubsystem.h"
#include "../InteriorInstanceSystem/InteriorSubsystem.h"
#include "../ActorStateSystem/ActorStateSubsystem.h"
#include "../InventorySystem/InventorySubsystem.h"

UInteractiveItemComponent::UInteractiveItemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
}

// Helper to get the subsystem and call AddRegistrationListener by SubsystemType
static void AddListenerToSubsystem(UWorld* World, EInteractiveSubsystem Type, const FGuid& ItemId, UInteractiveItemComponent* Comp)
{
	switch (Type)
	{
	case EInteractiveSubsystem::Terminal:
		if (UTerminalSubsystem* S = World->GetSubsystem<UTerminalSubsystem>())
			S->AddRegistrationListener(ItemId, Comp);
		break;
	case EInteractiveSubsystem::Interior:
		if (UInteriorSubsystem* S = World->GetSubsystem<UInteriorSubsystem>())
			S->AddRegistrationListener(ItemId, Comp);
		break;
	case EInteractiveSubsystem::ActorNPC:
		if (UGameInstance* GI = World->GetGameInstance())
			if (UActorStateSubsystem* S = GI->GetSubsystem<UActorStateSubsystem>())
				S->AddRegistrationListener(ItemId, Comp);
		break;
	case EInteractiveSubsystem::Inventory:
		if (UGameInstance* GI = World->GetGameInstance())
			if (UInventorySubsystem* S = GI->GetSubsystem<UInventorySubsystem>())
				S->AddRegistrationListener(ItemId, Comp);
		break;
	default: break;
	}
}

static void RemoveListenerFromSubsystem(UWorld* World, EInteractiveSubsystem Type, const FGuid& ItemId, UInteractiveItemComponent* Comp)
{
	switch (Type)
	{
	case EInteractiveSubsystem::Terminal:
		if (UTerminalSubsystem* S = World->GetSubsystem<UTerminalSubsystem>())
			S->RemoveRegistrationListener(ItemId, Comp);
		break;
	case EInteractiveSubsystem::Interior:
		if (UInteriorSubsystem* S = World->GetSubsystem<UInteriorSubsystem>())
			S->RemoveRegistrationListener(ItemId, Comp);
		break;
	case EInteractiveSubsystem::ActorNPC:
		if (UGameInstance* GI = World->GetGameInstance())
			if (UActorStateSubsystem* S = GI->GetSubsystem<UActorStateSubsystem>())
				S->RemoveRegistrationListener(ItemId, Comp);
		break;
	case EInteractiveSubsystem::Inventory:
		if (UGameInstance* GI = World->GetGameInstance())
			if (UInventorySubsystem* S = GI->GetSubsystem<UInventorySubsystem>())
				S->RemoveRegistrationListener(ItemId, Comp);
		break;
	default: break;
	}
}

void UInteractiveItemComponent::BeginPlay()
{
	Super::BeginPlay();

	ItemId = FGuid::NewGuid();
	CurrentTooltip = InteractiveTooltipText;

	UEventBusSubsystem* EventBus = GetWorld()->GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus) return;

	// Register per-item listener BEFORE publishing registration
	AddListenerToSubsystem(GetWorld(), SubsystemType, ItemId, this);

	// Publish registration payload
	UInteractItemRegistrationPayload* SpawnPayload = EventBus->CreatePayload<UInteractItemRegistrationPayload>();
	SpawnPayload->Setup(ItemId, SubsystemType, InteractionRange, InteractiveTooltipText, GetOwner());

	FOutcomeEventBase SpawnEvent;
	switch (SubsystemType)
	{
	case EInteractiveSubsystem::Terminal:
		SpawnEvent.OutcomeType = EOutcomeType::Terminal;
		SpawnEvent.OutcomeTerminal = EOutcomeTerminal::InteractRegistered;
		break;
	case EInteractiveSubsystem::ActorNPC:
		SpawnEvent.OutcomeType = EOutcomeType::Actor;
		SpawnEvent.OutcomeActor = EOutcomeActor::InteractRegistered;
		break;
	case EInteractiveSubsystem::Inventory:
		SpawnEvent.OutcomeType = EOutcomeType::Inventory;
		SpawnEvent.OutcomeInventory = EOutcomeInventory::InteractRegistered;
		break;
	default:
		SpawnEvent.OutcomeType = EOutcomeType::Interior;
		SpawnEvent.OutcomeInterior = EOutcomeInterior::InteractRegistered;
		break;
	}
	SpawnEvent.Payload = SpawnPayload;
	EventBus->PublishOutcome(SpawnEvent);

	UE_LOG(LogTemp, Log, TEXT("InteractiveItemComponent: Published registration ItemId=%s SubsystemType=%d"),
		*ItemId.ToString(), static_cast<int32>(SubsystemType));
}

void UInteractiveItemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unregister per-item listener (all types)
	RemoveListenerFromSubsystem(GetWorld(), SubsystemType, ItemId, this);

	UEventBusSubsystem* EventBus = GetWorld()
		? GetWorld()->GetGameInstance()->GetSubsystem<UEventBusSubsystem>()
		: nullptr;

	if (EventBus)
	{
		UInteractItemRegistrationPayload* Payload = EventBus->CreatePayload<UInteractItemRegistrationPayload>();
		Payload->Setup(ItemId, SubsystemType, InteractionRange, InteractiveTooltipText, GetOwner());

		FOutcomeEventBase Event;
		switch (SubsystemType)
		{
		case EInteractiveSubsystem::Terminal:
			Event.OutcomeType = EOutcomeType::Terminal;
			Event.OutcomeTerminal = EOutcomeTerminal::InteractUnregistered;
			break;
		case EInteractiveSubsystem::ActorNPC:
			Event.OutcomeType = EOutcomeType::Actor;
			Event.OutcomeActor = EOutcomeActor::InteractUnregistered;
			break;
		case EInteractiveSubsystem::Inventory:
			Event.OutcomeType = EOutcomeType::Inventory;
			Event.OutcomeInventory = EOutcomeInventory::InteractUnregistered;
			break;
		default:
			Event.OutcomeType = EOutcomeType::Interior;
			Event.OutcomeInterior = EOutcomeInterior::InteractUnregistered;
			break;
		}
		Event.Payload = Payload;
		EventBus->PublishOutcome(Event);

		UE_LOG(LogTemp, Log, TEXT("InteractiveItemComponent: Published unregistration ItemId=%s"),
			*ItemId.ToString());
	}

	Super::EndPlay(EndPlayReason);
}

void UInteractiveItemComponent::OnRegisteredBySubsystem(UInteractItemRegistrationPayload* Payload)
{
	if (!Payload) return;
	AActor* Owner = Payload->GetOwnerActor();
	UE_LOG(LogTemp, Log, TEXT("InteractiveItemComponent: Notified registered by subsystem - Owner=%s Range=%.1f"),
		Owner ? *Owner->GetName() : TEXT("Unknown"), Payload->GetInteractionRange());
}

void UInteractiveItemComponent::OnUnregisteredBySubsystem(UInteractItemRegistrationPayload* Payload)
{
	if (!Payload) return;
	AActor* Owner = Payload->GetOwnerActor();
	UE_LOG(LogTemp, Log, TEXT("InteractiveItemComponent: Notified unregistered by subsystem - Owner=%s"),
		Owner ? *Owner->GetName() : TEXT("Unknown"));
}

void UInteractiveItemComponent::OnInteractEnabledOutcome(const FOutcomeEventBase& Outcome)
{
	if (const UInteractItemStatePayload* P = Cast<UInteractItemStatePayload>(Outcome.Payload))
	{
		ApplyStateFromPayload(P);
	}
}

void UInteractiveItemComponent::ApplyStateFromPayload(const UInteractItemStatePayload* Payload)
{
	if (!Payload || Payload->GetItemId() != ItemId) return;

	bInteractionEnabled = Payload->IsInteractionEnabled();
	CurrentTooltip      = Payload->GetCurrentTooltip();
	InteractionRange    = Payload->GetInteractionRange();

	SetActive(bInteractionEnabled);
	OnInteractStateChanged.Broadcast(bInteractionEnabled, CurrentTooltip);

	UE_LOG(LogTemp, Log, TEXT("InteractiveItemComponent: State applied ItemId=%s Enabled=%s Tooltip=%s"),
		*ItemId.ToString(),
		bInteractionEnabled ? TEXT("true") : TEXT("false"),
		*CurrentTooltip.ToString());
}

void UInteractiveItemComponent::FinishInteractiveUse(ACharacter* IIUser, const bool IsReleaseButton)
{
	if (!IsValid(this) || !IsValid(IIUser)) return;
	ReleasedUser = IIUser;
	IsRelease = IsReleaseButton;
	OnInteractiveLostFocusEvent.Broadcast(ReleasedUser);
}

void UInteractiveItemComponent::SetIsInteractiveNow(AActor* WhoInteract)
{
	if (!IsValid(this)) return;
	OnInteractiveReceiveFocusEvent.Broadcast(WhoInteract);
}

void UInteractiveItemComponent::DoInteractiveUse(ACharacter* IIUser)
{
	if (!IsValid(this) || !IsActive() || !IsValid(IIUser)) return;
	auto Picker = Cast<UInteractivePickerComponent>(IIUser->GetComponentByClass(UInteractivePickerComponent::StaticClass()));
	OnInteractionPressKeyEvent.Broadcast(Picker);
}

void UInteractiveItemComponent::SetTooltip(const FText& NewTooltip)
{
	CurrentTooltip = NewTooltip;
	InteractiveTooltipText = NewTooltip;

	// 3) Уведомляем подписчиков о том, что тултип изменился
	if (OnInteractTooltipChange.IsBound())
	{
		OnInteractTooltipChange.Broadcast(CurrentTooltip);
	}

	UE_LOG(LogTemp, Log, TEXT("InteractiveItemComponent: Tooltip set ItemId=%s Tooltip=%s"),
		*ItemId.ToString(), *CurrentTooltip.ToString());
}
