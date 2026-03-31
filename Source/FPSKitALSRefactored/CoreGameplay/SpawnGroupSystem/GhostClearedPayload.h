#pragma once
#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "GhostClearedPayload.generated.h"

// Payload for GhostCleared events
// (Payload для событий GhostCleared)
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UGhostClearedPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "GhostCleared")
	FGuid MissionId;

	UPROPERTY(BlueprintReadWrite, Category = "GhostCleared")
	FString GhostType;

	UPROPERTY(BlueprintReadWrite, Category = "GhostCleared")
	FGuid InteriorSetId;

	UPROPERTY(BlueprintReadWrite, Category = "GhostCleared")
	FGuid SpawnGroupId;

	// Fill all fields in one call - use after CreatePayload
	// C++:  Payload->Setup(MissionId, GhostType, InteriorSetId, SpawnGroupId)
	// BP:   Setup node after CreatePayload → Cast To UGhostClearedPayload
	// (Заполняет все поля одним вызовом - использовать после CreatePayload)
	UFUNCTION(BlueprintCallable, Category = "GhostCleared")
	UGhostClearedPayload* Setup(
		const FGuid&   InMissionId,
		const FString& InGhostType,
		const FGuid&   InInteriorSetId,
		const FGuid&   InSpawnGroupId)
	{
		MissionId     = InMissionId;
		GhostType     = InGhostType;
		InteriorSetId = InInteriorSetId;
		SpawnGroupId  = InSpawnGroupId;
		return this;
	}

	// Getters
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GhostCleared")
	FGuid GetMissionId() const { return MissionId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GhostCleared")
	FString GetGhostType() const { return GhostType; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GhostCleared")
	FGuid GetInteriorSetId() const { return InteriorSetId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GhostCleared")
	FGuid GetSpawnGroupId() const { return SpawnGroupId; }
};