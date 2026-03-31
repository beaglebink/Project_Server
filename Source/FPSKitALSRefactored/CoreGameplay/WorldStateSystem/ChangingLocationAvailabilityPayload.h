#pragma once
#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "ChangingLocationAvailabilityPayload.generated.h"

// Payload for ChangingLocationAvailability events
// (Payload для событий доступности локаций)
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UChangingLocationAvailabilityPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "ChangingLocationAvailabilityPayload")
	FString LocationName;

	UPROPERTY(BlueprintReadWrite, Category = "ChangingLocationAvailabilityPayload")
	bool bIsAvailable = false;

	// Fill all fields in one call - use after CreatePayload
	// (Заполняет все поля одним вызовом - использовать после CreatePayload)
	UFUNCTION(BlueprintCallable, Category = "ChangingLocationAvailabilityPayload")
	UChangingLocationAvailabilityPayload* Setup(
		const FString& InLocationName,
		bool         bInIsAvailable)
	{
		LocationName  = InLocationName;
		bIsAvailable  = bInIsAvailable;
		return this;
	}

	// Getters
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ChangingLocationAvailabilityPayload")
	FString GetLocationName() const { return LocationName; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ChangingLocationAvailabilityPayload")
	bool IsAvailable() const { return bIsAvailable; }
};