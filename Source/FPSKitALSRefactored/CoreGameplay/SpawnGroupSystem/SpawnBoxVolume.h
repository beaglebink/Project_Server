// SpawnBoxVolume.h
#pragma once

#include "CoreMinimal.h"
#include "SpawnVolume.h"
#include "Components/BoxComponent.h"
#include "SpawnBoxVolume.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API ASpawnBoxVolume : public ASpawnVolume
{
    GENERATED_BODY()
public:
    ASpawnBoxVolume();
    virtual FVector GetRandomSpawnPoint_Implementation() const override;
    virtual FRotator GetDefaultRotation_Implementation() const override;
    UFUNCTION(BlueprintCallable, Category = "SpawnVolume")
    void SetBoxExtent(const FVector& Extent);
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> BoxComponent;
protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnVolume")
    FVector BoxExtent = FVector(100.0f, 100.0f, 100.0f);
};