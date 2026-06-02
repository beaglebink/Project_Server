#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnVolume.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API ASpawnVolume : public AActor
{
    GENERATED_BODY()

public:
    ASpawnVolume();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SpawnVolume")
    FVector GetRandomSpawnPoint() const;
    virtual FVector GetRandomSpawnPoint_Implementation() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SpawnVolume")
    FRotator GetDefaultRotation() const;
    virtual FRotator GetDefaultRotation_Implementation() const;
};