#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "FloorStatePayload.generated.h"

class UFloorAsset; // forward

/** Payload for FloorStateSave and FloorStateRestore commands. */
// (Payload для команд FloorStateSave и FloorStateRestore.)
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UFloorStatePayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	// runtime-friendly selector (preferred)
	UPROPERTY(BlueprintReadWrite, Category = "FloorState")
	FString InteriorSetPath; // e.g. "/Game/Content/Location/InteriorSet_MyHouse" or just asset name "InteriorSet_MyHouse"

	UPROPERTY(BlueprintReadWrite, Category = "FloorState")
	int32 FloorIndex = -1;

	// resolved GUIDs (filled by Setup)
	UPROPERTY(BlueprintReadWrite, Category = "FloorState")
	FGuid InteriorSetId;

	UPROPERTY(BlueprintReadWrite, Category = "FloorState")
	FGuid FloorId;

    // Setup helper: accepts asset path or name + floor index; resolves GUIDs internally.
    // (Setup helper: принимает путь или имя ассета + индекс этажа; внутри определяет GUID'ы.)
    UFUNCTION(BlueprintCallable, Category = "FloorState")
    UFloorStatePayload* Setup(const FString& InInteriorSetPathOrName, int32 InFloorIndex);

    // New convenient Setup: provide the Floor asset directly (recommended).
    // In Blueprints you can pass a reference to the FloorAsset directly.
    // (Новый удобный Setup: задавать этаж через сам Asset (рекомендуется). В Blueprint можно передать ссылку на FloorAsset напрямую.)
    UFUNCTION(BlueprintCallable, Category = "FloorState")
    UFloorStatePayload* SetupFromFloorAsset(UFloorAsset* InFloorAsset);
};