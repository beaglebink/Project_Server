#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeQuery.h"
#include "Containers/List.h"
#include "EventBusSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOutcomeEvent, const FOutcomeEventBase&, Outcome);

// Event handler signature (Сигнатура обработчика события)
using FOutcomeHandlerDelegate = TDelegate<void(const FOutcomeEventBase&)>;

// Structure to store handler with condition (Структура для хранения обработчика с условием)
struct FOutcomeHandlerEntry
{
	FOutcomeHandlerDelegate Handler;
	TSharedPtr<IOutcomeCondition> Query;

	FOutcomeHandlerEntry(FOutcomeHandlerDelegate InHandler, TSharedPtr<IOutcomeCondition> InQuery)
		: Handler(InHandler), Query(InQuery)
	{
	}
};

UCLASS()
class FPSKITALSREFACTORED_API UEventBusSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnOutcomeEvent OnOutcomeEvent;

	// Publish outcome event to all registered handlers (Опубликовать событие результата всем зарегистрированным обработчикам)
	UFUNCTION(BlueprintCallable, Category = "EventBus")
	void PublishOutcome(const FOutcomeEventBase& Outcome);

	/**
	 * Register handler with conditions
	 * @param Handler - handler function
	 * @param Query - event filtering condition (can be nullptr to process all events)
	 * 
	 * (Регистрация обработчика с условиями
	 * @param Handler - функция обработчик
	 * @param Query - условие фильтрации события (может быть nullptr для обработки всех событий))
	 */
	void RegisterHandler(FOutcomeHandlerDelegate Handler, TSharedPtr<IOutcomeCondition> Query = nullptr)
	{
		Handlers.Add(FOutcomeHandlerEntry(Handler, Query));
	}

	/**
	 * Quick registration version by event type only
	 * Kept for backward compatibility
	 * 
	 * (Версия для быстрой регистрации только по типу события
	 * Оставлена для обратной совместимости)
	 */
	void RegisterHandlerByType(EOutcomeType OutcomeType, FOutcomeHandlerDelegate Handler)
	{
		RegisterHandler(Handler, FOutcomeQueryBuilder::Type(OutcomeType));
	}

	// Clear all registered handlers (Очистка всех обработчиков)
	UFUNCTION(BlueprintCallable, Category = "EventBus")
	void ClearAllHandlers()
	{
		Handlers.Empty();
	}

	// Get count of registered handlers (Получить количество зарегистрированных обработчиков)
	UFUNCTION(BlueprintCallable, Category = "EventBus")
	int32 GetHandlerCount() const
	{
		return Handlers.Num();
	}

private:
	// List of handlers with conditions (Список обработчиков с условиями)
	TArray<FOutcomeHandlerEntry> Handlers;
};
