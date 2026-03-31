#include "WorldStateSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "ChangingLocationAvailabilityPayload.h"

void UWorldStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Do not force subscription here — keep it lazy. If the condition asset is already set in editor, subscribe now.
	// (Не подписываемся здесь принудительно — оставляем подписку ленивой. Если ассет уже задан в редакторе, можно подписаться прямо.)
	if (ChangingLocationAvailabilityCondition)
	{
		SubscribeChangingLocationAvailability();
	}
}

void UWorldStateSubsystem::Deinitialize()
{
	UnsubscribeAll();
	Super::Deinitialize();
}

void UWorldStateSubsystem::SetChangingLocationAvailabilityCondition(UOutcomeConditionAsset* NewCondition)
{
	// If the condition did not change, do nothing.
	// (Если условие не изменилось — ничего не делаем.)
	if (ChangingLocationAvailabilityCondition == NewCondition) return;

	// Save the new condition.
	// (Сохраняем новое условие.)
	ChangingLocationAvailabilityCondition = NewCondition;

	// If already subscribed, update registration.
	// (Если уже подписаны — обновляем регистрацию.)
	if (ChangingLocationAvailabilityHandle.IsValid())
	{
		UnsubscribeChangingLocationAvailability();
	}

	// Try to subscribe immediately if EventBus is available.
	// (Попробуем подписаться сразу, если EventBus доступен.)
	SubscribeChangingLocationAvailability();
}

void UWorldStateSubsystem::SubscribeChangingLocationAvailability()
{
	// Already subscribed — nothing to do.
	// (Уже подписаны — ничего делать не нужно.)
	if (ChangingLocationAvailabilityHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("WorldStateSubsystem: Already subscribed to ChangingLocationAvailability"));
		return;
	}

	// Condition asset is required to subscribe.
	// (Для подписки требуется ассет условия.)
	if (!ChangingLocationAvailabilityCondition)
	{
		UE_LOG(LogTemp, Warning, TEXT("WorldStateSubsystem: ChangingLocationAvailabilityCondition is null, cannot subscribe (call SetChangingLocationAvailabilityCondition after assigning asset)"));
		return;
	}

	// Register handler with EventBus subsystem if available.
	// (Регистрируем обработчик в EventBus, если он доступен.)
	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		ChangingLocationAvailabilityHandle = EventBus->RegisterHandler(
			ChangingLocationAvailabilityCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UWorldStateSubsystem::HandleChangingLocationAvailability)
		);

		UE_LOG(LogTemp, Log, TEXT("WorldStateSubsystem: Subscribed to ChangingLocationAvailability (handle=%u)"), ChangingLocationAvailabilityHandle.GetId());
	}
}

void UWorldStateSubsystem::UnsubscribeChangingLocationAvailability()
{
	// If handle is invalid — nothing to unsubscribe.
	// (Если дескриптор невалидный — нечего отписывать.)
	if (!ChangingLocationAvailabilityHandle.IsValid()) return;

	// Unregister handler from EventBus and invalidate handle.
	// (Снимаем регистрацию с EventBus и инвалиируем дескриптор.)
	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		EventBus->UnregisterHandler(ChangingLocationAvailabilityHandle);
		UE_LOG(LogTemp, Log, TEXT("WorldStateSubsystem: Unsubscribed ChangingLocationAvailability (handle=%u)"), ChangingLocationAvailabilityHandle.GetId());
	}
	ChangingLocationAvailabilityHandle.Invalidate();
}

void UWorldStateSubsystem::UnsubscribeAll()
{
	// Unsubscribe all related handlers.
	// (Отписать все связанные обработчики.)
	UnsubscribeChangingLocationAvailability();
}

void UWorldStateSubsystem::HandleChangingLocationAvailability(const FOutcomeEventBase& Outcome)
{
	// Defensive check: verify that the incoming outcome satisfies the condition asset.
	// (Защитная проверка: удостовериться, что входящее событие соответствует ассету условия.)
	if (ChangingLocationAvailabilityCondition)
	{
		// Use already compiled query (do not force recompile).
		// (Используем уже скомпилированную Query, не форсируем повторную компиляцию.)
		auto Query = ChangingLocationAvailabilityCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome))
		{
			UE_LOG(LogTemp, Verbose, TEXT("WorldStateSubsystem: Incoming outcome does not satisfy ChangingLocationAvailabilityCondition -> ignoring"));
			return;
		}
	}

	// Try to cast Payload to expected type and log.
	// (Пробуем привести Payload к ожидаемому типу и залогировать.)
	if (UChangingLocationAvailabilityPayload* P = Cast<UChangingLocationAvailabilityPayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("WorldStateSubsystem: Location - Name: %s, IsAvailable: %s"),
			*P->LocationName, P->bIsAvailable ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("WorldStateSubsystem: HandleChangingLocationAvailability called with no payload or unexpected payload type"));
	}

	// Broadcast the unified event to Blueprint listeners.
	// (Шлём унифицированное событие слушателям Blueprint.)
	OnChangingLocationAvailability.Broadcast(Outcome);
}


