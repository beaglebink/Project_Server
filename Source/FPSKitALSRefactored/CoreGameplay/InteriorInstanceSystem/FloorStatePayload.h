#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "FloorStatePayload.generated.h"

class UFloorAsset; // forward

/** Payload для команд FloorStateSave и FloorStateRestore. */
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

	// Setup helper: принимает путь или имя ассета + индекс этажа; внутри определяет GUID'ы.
	UFUNCTION(BlueprintCallable, Category = "FloorState")
	UFloorStatePayload* Setup(const FString& InInteriorSetPathOrName, int32 InFloorIndex);

	// Новый удобный Setup: задавать этаж через сам Asset (рекомендуется).
	// В Blueprint можно передать ссылку на FloorAsset напрямую.
	UFUNCTION(BlueprintCallable, Category = "FloorState")
	UFloorStatePayload* SetupFromFloorAsset(UFloorAsset* InFloorAsset);
};