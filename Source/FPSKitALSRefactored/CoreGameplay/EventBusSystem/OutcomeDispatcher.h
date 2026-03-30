#pragma once

#include "CoreMinimal.h"
#include "OutcomeHandler.h"
#include "Containers/Map.h"

struct FOutcomeEventBase;

// Central event dispatcher (Центральный диспетчер событий)
class FOutcomeDispatcher
{
public:
	// Register handler (Регистрация обработчика)
	template<typename OutcomeType>
	void Register(TFunction<void(const OutcomeType&)> Handler)
	{
		int32 TypeHash = GetTypeHash(OutcomeType::StaticStruct());
		Handlers.FindOrAdd(TypeHash).Add(MakeShared<TOutcomeHandler<OutcomeType>>(Handler));
	}

	// Dispatch event (Отправка события)
	void Dispatch(const FOutcomeEventBase& Outcome)
	{
		int32 TypeHash = GetTypeHash(Outcome.GetType());
		if (TArray<TSharedPtr<IOutcomeHandler>>* HandlersArray = Handlers.Find(TypeHash))
		{
			for (const auto& Handler : *HandlersArray)
			{
				if (Handler->CanHandle(Outcome))
				{
					Handler->Handle(Outcome);
				}
			}
		}
	}

	// Clear all handlers (Очистить все обработчики)
	void Clear() { Handlers.Empty(); }

private:
	// Map of type hash to handlers array (Карта хеша типа к массиву обработчиков)
	TMap<int32, TArray<TSharedPtr<IOutcomeHandler>>> Handlers;
};