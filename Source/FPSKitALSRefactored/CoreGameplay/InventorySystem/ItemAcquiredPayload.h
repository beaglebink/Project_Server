#pragma once
#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "ItemAcquiredPayload.generated.h"

// Payload for ItemAcquired events
// (Payload для событий получения предметов)
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UItemAcquiredPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "ItemAcquired")
	FGuid ActorId;

	UPROPERTY(BlueprintReadWrite, Category = "ItemAcquired")
	FGuid ObjectId;

	// Fill all fields in one call - use after CreatePayload
	// (Заполняет все поля одним вызовом - использовать после CreatePayload)
	UFUNCTION(BlueprintCallable, Category = "ItemAcquired")
	UItemAcquiredPayload* Setup(
		const FGuid& InActorId,
		const FGuid& InObjectId)
	{
		ActorId  = InActorId;
		ObjectId = InObjectId;
		return this;
	}

	// Getters
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ItemAcquired")
	FGuid GetActorId() const { return ActorId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ItemAcquired")
	FGuid GetObjectId() const { return ObjectId; }
};