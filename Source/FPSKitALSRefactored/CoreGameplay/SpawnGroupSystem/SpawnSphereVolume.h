#pragma once

#include "CoreMinimal.h"
#include "SpawnVolume.h"
#include "Components/SphereComponent.h"
#include "SpawnSphereVolume.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API ASpawnSphereVolume : public ASpawnVolume
{
    GENERATED_BODY()

public:
    ASpawnSphereVolume();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> SphereComponent;

    UFUNCTION(BlueprintCallable, Category = "SpawnVolume")
    void SetSphereRadius(float Radius);

    virtual FVector GetRandomSpawnPoint_Implementation() const override;
    virtual FRotator GetDefaultRotation_Implementation() const override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnVolume")
    float SphereRadius = 100.0f;
};