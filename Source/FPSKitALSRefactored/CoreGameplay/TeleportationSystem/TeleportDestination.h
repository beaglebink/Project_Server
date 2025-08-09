#pragma once

#include "CoreMinimal.h"

#include "TeleportDestination.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API ATeleportDestination : public AActor
{
	GENERATED_BODY()
public:
	void BeginPlay() override;

	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
	FString DestinationID;
};