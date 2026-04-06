#pragma once

#include "CoreMinimal.h"
#include "Outcome.h"
#include "OutcomePayload.h"
#include "OutcomeEventBase.generated.h"

// Lightweight event struct carrying filter fields and optional payload
USTRUCT(BlueprintType)
struct FOutcomeEventBase
{
	GENERATED_BODY()

	// ===== FILTER FIELDS - used by OutcomeConditionAsset =====
	UPROPERTY(BlueprintReadWrite, Category = "Outcome")
	EOutcomeType OutcomeType = EOutcomeType::Default;

	UPROPERTY(BlueprintReadWrite, Category = "Outcome")
	EOutcomeMission OutcomeMission = EOutcomeMission::Default;

	UPROPERTY(BlueprintReadWrite, Category = "Outcome")
	EOutcomeActor OutcomeActor = EOutcomeActor::Default;

	// renamed: OutcomeInventory replaces former OutcomeObject
	UPROPERTY(BlueprintReadWrite, Category = "Outcome")
	EOutcomeInventory OutcomeInventory = EOutcomeInventory::Default;

	UPROPERTY(BlueprintReadWrite, Category = "Outcome")
	EOutcomeTerminal OutcomeTerminal = EOutcomeTerminal::Default;

	UPROPERTY(BlueprintReadWrite, Category = "Outcome")
	EOutcomeInterior OutcomeInterior = EOutcomeInterior::Default;

	UPROPERTY(BlueprintReadWrite, Category = "Outcome")
	EOutcomeSpawnGroup OutcomeSpawnGroup = EOutcomeSpawnGroup::Default;

	// ------- ВОССТАНОВЛЕНО: поле состояния мира, ожидаемое OutcomeQuery и другими частями кода
	UPROPERTY(BlueprintReadWrite, Category = "Outcome")
	EWorldState WorldState = EWorldState::Default;

	// ===== optional payload =====
	UPROPERTY(BlueprintReadWrite, Category = "Outcome")
	TObjectPtr<UOutcomePayload> Payload = nullptr;
};
