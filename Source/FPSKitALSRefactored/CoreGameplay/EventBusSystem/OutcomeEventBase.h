#pragma once
#include "CoreMinimal.h"
#include "Outcome.h"
#include "OutcomePayload.h"
#include "OutcomeEventBase.generated.h"

// Base outcome event structure
// Filter fields: enum categories (used by OutcomeConditionAsset)
// Extra data: Payload UObject - cast to concrete type in handler
// (Базовая структура события)
// (Поля фильтрации: enum категории - используются OutcomeConditionAsset)
// (Дополнительные данные: Payload UObject - кастовать к конкретному типу в обработчике)
USTRUCT(BlueprintType)
struct FOutcomeEventBase
{
	GENERATED_BODY()

	// ===== FILTER FIELDS - used by OutcomeConditionAsset =====
	// (Поля фильтрации - используются OutcomeConditionAsset)

	UPROPERTY(BlueprintReadWrite)
	EOutcomeType OutcomeType = EOutcomeType::Default;

	UPROPERTY(BlueprintReadWrite)
	EOutcomeMission OutcomeMission = EOutcomeMission::Default;

	UPROPERTY(BlueprintReadWrite)
	EOutcomeActor OutcomeActor = EOutcomeActor::Default;

	UPROPERTY(BlueprintReadWrite)
	EOutcomeObject OutcomeObject = EOutcomeObject::Default;

	UPROPERTY(BlueprintReadWrite)
	EOutcomeTerminal OutcomeTerminal = EOutcomeTerminal::Default;

	UPROPERTY(BlueprintReadWrite)
	EOutcomeInterior OutcomeInterior = EOutcomeInterior::Default;

	UPROPERTY(BlueprintReadWrite)
	EOutcomeSpawnGroup OutcomeSpawnGroup = EOutcomeSpawnGroup::Default;

	UPROPERTY(BlueprintReadWrite)
	EWorldState WorldState = EWorldState::Default;

	// ===== PAYLOAD - extra data for handlers =====
	// C++:       Cast<UMyPayload>(Outcome.Payload)
	// Blueprint: Cast To <BP_MyPayload> node on Outcome.Payload
	// nullptr if no extra data needed
	// (Дополнительные данные для обработчиков)
	// (nullptr если дополнительные данные не нужны)
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UOutcomePayload> Payload = nullptr;
};
