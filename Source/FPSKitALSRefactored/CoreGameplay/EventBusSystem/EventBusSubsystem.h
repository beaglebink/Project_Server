#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeQuery.h"
#include "EventBusSubsystem.generated.h"

class UOutcomeConditionAsset;

// C++ handler delegate
using FOutcomeHandlerDelegate = TDelegate<void(const FOutcomeEventBase&)>;

// Internal storage for each registered handler
struct FOutcomeHandlerEntry
{
	FOutcomeHandlerDelegate Handler;
	TSharedPtr<IOutcomeCondition> Query;

	FOutcomeHandlerEntry(FOutcomeHandlerDelegate InHandler, TSharedPtr<IOutcomeCondition> InQuery)
		: Handler(MoveTemp(InHandler))
		, Query(InQuery)
	{
	}
};

UCLASS()
class FPSKITALSREFACTORED_API UEventBusSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Publish event - dispatches to all handlers whose conditions are met
	// (Опубликовать событие - рассылает всем обработчикам чьи условия выполнены)
	UFUNCTION(BlueprintCallable, Category = "EventBus")
	void PublishOutcome(const FOutcomeEventBase& Outcome);

	// Register C++ handler with condition asset - compiles asset automatically
	// (Зарегистрировать C++ обработчик с Asset условия - компилирует Asset автоматически)
	void RegisterHandler(UOutcomeConditionAsset* ConditionAsset, FOutcomeHandlerDelegate Handler);

private:
	TArray<FOutcomeHandlerEntry> Handlers;
};
