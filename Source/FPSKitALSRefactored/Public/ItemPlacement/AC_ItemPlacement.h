#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AC_ItemPlacement.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FPSKITALSREFACTORED_API UAC_ItemPlacement : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAC_ItemPlacement();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
};
