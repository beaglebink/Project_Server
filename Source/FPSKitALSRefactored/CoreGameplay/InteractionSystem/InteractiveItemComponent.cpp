#include "InteractiveItemComponent.h"
#include "InteractivePickerComponent.h"
#include "InteractItemStatePayload.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "../TerminalSystem/TerminalSubsystem.h"

UInteractiveItemComponent::UInteractiveItemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
}

void UInteractiveItemComponent::BeginPlay()
{
	Super::BeginPlay();

	// Generate unique ID for this item for the lifetime of this component
	ItemId = FGuid::NewGuid();
	CurrentTooltip = InteractiveTooltipText;

	UEventBusSubsystem* EventBus = GetWorld()->GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus) return;

	// Register per-item listener with subsystem BEFORE publishing registration so only this component gets notified.
	if (SubsystemType == EInteractiveSubsystem::Terminal)
	{
		if (UTerminalSubsystem* TS = GetWorld()->GetSubsystem<UTerminalSubsystem>())
		{
			TS->AddRegistrationListener(ItemId, this);
		}
	}

	// Publish registration payload — subsystem will process and then notify this component via per-item listener
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
		SpawnEvent.OutcomeType = EOutcomeType::Object;
		SpawnEvent.OutcomeObject = EOutcomeObject::InteractRegistered;
		break;

	case EInteractiveSubsystem::Interior:
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
	// Unregister per-item listener
	if (SubsystemType == EInteractiveSubsystem::Terminal)
	{
		if (UTerminalSubsystem* TS = GetWorld()->GetSubsystem<UTerminalSubsystem>())
		{
			TS->RemoveRegistrationListener(ItemId, this);
		}
	}

	UEventBusSubsystem* EventBus = GetWorld()
		? GetWorld()->GetGameInstance()->GetSubsystem<UEventBusSubsystem>()
		: nullptr;

	if (EventBus)
	{
		// publish unregistration using matching category token InteractUnregistered
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
			Event.OutcomeType = EOutcomeType::Object;
			Event.OutcomeObject = EOutcomeObject::InteractUnregistered;
			break;

		case EInteractiveSubsystem::Interior:
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
	const FString OwnerName = Owner ? Owner->GetName() : FString(TEXT("Unknown"));

	UE_LOG(LogTemp, Log, TEXT("InteractiveItemComponent: Notified registered by subsystem - Owner=%s Range=%.1f"),
		*OwnerName, Payload->GetInteractionRange());

	// Additional per-component setup can be done here if needed
}

void UInteractiveItemComponent::OnUnregisteredBySubsystem(UInteractItemRegistrationPayload* Payload)
{
	if (!Payload) return;

	AActor* Owner = Payload->GetOwnerActor();
	const FString OwnerName = Owner ? Owner->GetName() : FString(TEXT("Unknown"));

	UE_LOG(LogTemp, Log, TEXT("InteractiveItemComponent: Notified unregistered by subsystem - Owner=%s"),
		*OwnerName);
}

void UInteractiveItemComponent::OnInteractEnabledOutcome(const FOutcomeEventBase& Outcome)
{
	// Legacy: keep applying state when payload arrives (subsystem may still publish InteractEnabled)
	if (const UInteractItemStatePayload* P = Cast<UInteractItemStatePayload>(Outcome.Payload))
	{
		ApplyStateFromPayload(P);
	}
}

void UInteractiveItemComponent::ApplyStateFromPayload(const UInteractItemStatePayload* Payload)
{
	if (!Payload || Payload->GetItemId() != ItemId) return;

	bInteractionEnabled = Payload->IsInteractionEnabled();
	CurrentTooltip = Payload->GetCurrentTooltip();
	InteractionRange = Payload->GetInteractionRange();

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
	if (!IsValid(this) || !IsActive()) return;

	auto Picker = Cast<UInteractivePickerComponent>(IIUser->GetComponentByClass(UInteractivePickerComponent::StaticClass()));

	OnInteractionPressKeyEvent.Broadcast(Picker);
}

void UInteractiveItemComponent::SetIsActive(bool Active)
{
	SetActive(Active);
}
