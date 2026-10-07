#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StreetTransitionTrigger.generated.h"

class UBoxComponent;
class UStreetAsset;

UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API AStreetTransitionTrigger : public AActor
{
    GENERATED_BODY()

public:
    AStreetTransitionTrigger();

    // Улица, на которую переходит игрок, попадая в этот триггер.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
    TObjectPtr<UStreetAsset> TargetStreet;

protected:
    virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> TriggerVolume;
};