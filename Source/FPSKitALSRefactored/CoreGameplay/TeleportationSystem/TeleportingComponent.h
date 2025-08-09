#pragma once

#include "TeleportingComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPSKITALSREFACTORED_API UTeleportingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTeleportingComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
	FString ObjectID;
};