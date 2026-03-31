#pragma once
#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "ActorStatePayload.generated.h"

// Payload for ActorState events
// (Payload для событий состояния актора)
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UActorStatePayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "ActorState")
	FGuid ActorId;

	UPROPERTY(BlueprintReadWrite, Category = "ActorState")
	FString StateChangeType;

	UPROPERTY(BlueprintReadWrite, Category = "ActorState")
	FString ContextId;

	UPROPERTY(BlueprintReadWrite, Category = "ActorState")
	TMap<FString, FString> ExtraData;

	// Fill base fields in one call - use after CreatePayload
	// ExtraData can be populated separately via Add to Map node in Blueprint
	// (Заполняет базовые поля одним вызовом - использовать после CreatePayload)
	// (ExtraData заполняется отдельно через Add to Map в Blueprint)
	UFUNCTION(BlueprintCallable, Category = "ActorState")
	UActorStatePayload* Setup(
		const FGuid&   InActorId,
		const FString& InStateChangeType,
		const FString& InContextId)
	{
		ActorId         = InActorId;
		StateChangeType = InStateChangeType;
		ContextId       = InContextId;
		return this;
	}

	// Add a single key-value pair to ExtraData
	// Chain calls: Setup(...)->AddExtra("key", "value")->AddExtra(...)
	// (Добавляет пару ключ-значение в ExtraData, можно чейнить вызовы)
	UFUNCTION(BlueprintCallable, Category = "ActorState")
	UActorStatePayload* AddExtra(const FString& Key, const FString& Value)
	{
		ExtraData.Add(Key, Value);
		return this;
	}

	// Getters
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ActorState")
	FGuid GetActorId() const { return ActorId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ActorState")
	FString GetStateChangeType() const { return StateChangeType; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ActorState")
	FString GetContextId() const { return ContextId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ActorState")
	TMap<FString, FString> GetExtraData() const { return ExtraData; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ActorState")
	FString GetExtraValue(const FString& Key) const
	{
		const FString* Found = ExtraData.Find(Key);
		return Found ? *Found : FString();
	}
};