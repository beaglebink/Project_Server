#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomePayload.h"
#include "OutcomeQuery.h"
#include "EventBusSubsystem.generated.h"

class UOutcomeConditionAsset;

// C++ handler delegate
// (Делегат C++ обработчика)
using FOutcomeHandlerDelegate = TDelegate<void(const FOutcomeEventBase&)>;

// Handle returned by RegisterHandler
// (Дескриптор возвращаемый RegisterHandler)
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

// Internal handler entry
// (Внутренняя запись обработчика)
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
	// Publish event - filter fields in Outcome, extra data in Outcome.Payload
	// C++:  EventBus->PublishOutcome(Event)  where Event.Payload = MyPayload
	// BP:   PublishOutcome node, set Payload field before calling
	// (Публикует событие - поля фильтрации в Outcome, доп. данные в Outcome.Payload)
	UFUNCTION(BlueprintCallable, Category = "EventBus")
	void PublishOutcome(const FOutcomeEventBase& Outcome);

	// Create a typed Payload object owned by GameInstance (no GC risk)
	// Use this to create payload before PublishOutcome
	// C++:  UMyPayload* P = EventBus->CreatePayload<UMyPayload>()
	// BP:   CreatePayload node → Cast To <BP_MyPayload>
	// (Создаёт типизированный Payload с GameInstance как владельцем - нет риска GC)
	UFUNCTION(BlueprintCallable, Category = "EventBus", meta = (DeterminesOutputType = "PayloadClass"))
	UOutcomePayload* CreatePayload(TSubclassOf<UOutcomePayload> PayloadClass);

	// C++-only typed version of CreatePayload
	// (C++-only типизированная версия CreatePayload)
	template<typename T>
	T* CreatePayload()
	{
		static_assert(TIsDerivedFrom<T, UOutcomePayload>::IsDerived,
			"T must derive from UOutcomePayload");
		return NewObject<T>(GetGameInstance());
	}

	// Register C++ handler
	// (Регистрирует C++ обработчик)
	FOutcomeHandlerHandle RegisterHandler(
		UOutcomeConditionAsset* ConditionAsset,
		FOutcomeHandlerDelegate Handler);

	// Unregister handler by handle
	// (Снимает обработчик по дескриптору)
	void UnregisterHandler(FOutcomeHandlerHandle& Handle);

	UFUNCTION(BlueprintCallable, Category = "EventBus")
	int32 GetHandlerCount() const { return Handlers.Num(); }

	// Override BeginDestroy to perform cleanup if needed (implementation in .cpp)
	virtual void BeginDestroy() override;

private:
	TArray<FOutcomeHandlerEntry> Handlers;
	uint32 NextHandleId = 1;

	// Защита от reentrant Register/Unregister во время PublishOutcome
	bool bDispatching = false;

	struct FPendingOperation
	{
		enum class EType : uint8 { Register, Unregister } Type = EType::Unregister;
		uint32 HandleId = 0;
		FOutcomeHandlerEntry Entry{ 0, FOutcomeHandlerDelegate{}, nullptr };
	};
	TArray<FPendingOperation> PendingOperations;
};
