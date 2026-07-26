// GhostCapturedPayload.h
#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "../InteriorInstanceSystem/FloorAssignmentComponent.h" // <-- добавлено
#include "GhostCapturedPayload.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UGhostCapturedPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "GhostCaptured")
    TWeakObjectPtr<AActor> CapturedActor;

    UPROPERTY(BlueprintReadWrite, Category = "GhostCaptured")
    FGuid ItemId;

    UFUNCTION(BlueprintCallable, Category = "GhostCaptured")
    UGhostCapturedPayload* Setup(AActor* InActor)
    {
        CapturedActor = InActor;
        if (InActor)
        {
            if (UFloorAssignmentComponent* Comp = InActor->FindComponentByClass<UFloorAssignmentComponent>())
                ItemId = Comp->ItemId;
        }
        return this;
    }
};