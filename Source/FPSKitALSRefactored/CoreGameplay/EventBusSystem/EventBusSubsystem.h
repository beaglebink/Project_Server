#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeQuery.h"
#include "EventBusSubsystem.generated.h"

class UOutcomeConditionAsset;

// C++ handler delegate (only C++ - not accessible from Blueprint)
// (Делегат C++ обработчика - только C++, недоступен из Blueprint)
using FOutcomeHandlerDelegate = TDelegate<void(const FOutcomeEventBase&)>;

// Handle returned by RegisterHandler - used to unregister later
// Store in subsystem, pass to UnregisterHandler in Deinitialize
// (Дескриптор возвращаемый RegisterHandler - используется для отписки)
// (Хранить в подсистеме, передавать в UnregisterHandler в Deinitialize)
struct FOutcomeHandlerHandle
{
	static const uint32 Invalid = 0;

	FOutcomeHandlerHandle() : Id(Invalid) {}
	explicit FOutcomeHandlerHandle(uint32 InId) : Id(InId) {}

	bool IsValid() const { return Id != Invalid; }
	void Invalidate() { Id = Invalid; }
	uint32 GetId() const { return Id; }

	bool operator==(const FOutcomeHandlerHandle& Other) const { return Id == Other.Id; }
	bool operator!=(const FOutcomeHandlerHandle& Other) const { return Id != Other.Id; }

private:
	uint32 Id;
};

// Internal storage entry for each registered handler
// (Внутренняя запись хранилища для каждого зарегистрированного обработчика)
struct FOutcomeHandlerEntry
{
	uint32 HandleId;
	FOutcomeHandlerDelegate Handler;
	TSharedPtr<IOutcomeCondition> Query;

	FOutcomeHandlerEntry(uint32 InHandleId, FOutcomeHandlerDelegate InHandler,
		TSharedPtr<IOutcomeCondition> InQuery)
		: HandleId(InHandleId)
		, Handler(MoveTemp(InHandler))
		, Query(InQuery)
	{
	}
};

UCLASS()
class FPSKITALSREFACTORED_API UEventBusSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Publish event to all handlers whose conditions are met
	// (Опубликовать событие всем обработчикам чьи условия выполнены)
	UFUNCTION(BlueprintCallable, Category = "EventBus")
	void PublishOutcome(const FOutcomeEventBase& Outcome);

	// Register C++ handler with condition asset
	// Returns handle - store it to unregister later
	// (Зарегистрировать C++ обработчик с Asset условия)
	// (Возвращает дескриптор - сохранить для последующей отписки)
	FOutcomeHandlerHandle RegisterHandler(
		UOutcomeConditionAsset* ConditionAsset,
		FOutcomeHandlerDelegate Handler);

	// Unregister handler by handle - invalidates handle after removal
	// (Отписать обработчик по дескриптору - инвалидирует дескриптор после удаления)
	void UnregisterHandler(FOutcomeHandlerHandle& Handle);

	// Get count of registered handlers for debugging
	// (Получить количество обработчиков для отладки)
	UFUNCTION(BlueprintCallable, Category = "EventBus")
	int32 GetHandlerCount() const { return Handlers.Num(); }

private:
	TArray<FOutcomeHandlerEntry> Handlers;
	uint32 NextHandleId = 1;
};
