#pragma once

#include "CoreMinimal.h"

struct FOutcomeEventBase;

// Event handler interface (Интерфейс обработчика события)
class IOutcomeHandler
{
public:
	virtual ~IOutcomeHandler() = default;
	
	// Check if handler can process this outcome (Проверить может ли обработчик обработать этот результат)
	virtual bool CanHandle(const FOutcomeEventBase& Outcome) const = 0;
	
	// Handle the outcome event (Обработать событие результата)
	virtual void Handle(const FOutcomeEventBase& Outcome) = 0;
};

// Typed handler with template specialization (Типизированный обработчик с специализацией шаблона)
template<typename OutcomeType>
class TOutcomeHandler : public IOutcomeHandler
{
public:
	// Handler function type (Тип функции обработчика)
	using HandlerFunc = TFunction<void(const OutcomeType&)>;

	TOutcomeHandler(HandlerFunc InHandler) : Handler(InHandler) {}

	// Check if this handler can process the outcome (Проверить может ли этот обработчик обработать результат)
	virtual bool CanHandle(const FOutcomeEventBase& Outcome) const override
	{
		return Outcome.IsA<OutcomeType>();
	}

	// Execute handler with typed outcome (Выполнить обработчик с типизированным результатом)
	virtual void Handle(const FOutcomeEventBase& Outcome) override
	{
		if (const OutcomeType* TypedOutcome = static_cast<const OutcomeType*>(&Outcome))
		{
			Handler(*TypedOutcome);
		}
	}

private:
	// Handler function (Функция обработчика)
	HandlerFunc Handler;
};