#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "I_PortalInteraction.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UI_PortalInteraction : public UInterface
{
    GENERATED_BODY()
};

class ALSEXTRAS_API II_PortalInteraction
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PortalInterface")
    void PortalInteract(const FHitResult& Hit);
};