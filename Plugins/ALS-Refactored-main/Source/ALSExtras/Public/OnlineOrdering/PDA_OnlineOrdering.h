#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "OnlineOrdering/O_OnlineOrderingStructures.h"
#include "PDA_OnlineOrdering.generated.h"

UCLASS()
class ALSEXTRAS_API UPDA_OnlineOrdering : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OnlineOrders")
	FOnlineOrderingData OrderData;
};
