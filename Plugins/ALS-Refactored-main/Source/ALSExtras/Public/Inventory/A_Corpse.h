#pragma once

#include "CoreMinimal.h"
#include "Inventory/A_BaseInteractiveContainer.h"
#include "A_Corpse.generated.h"

UCLASS()
class ALSEXTRAS_API AA_Corpse : public AA_BaseInteractiveContainer
{
	GENERATED_BODY()
	
public:	
	AA_Corpse();

protected:

	virtual void TimelineProgress(float Value) override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void Open(UInteractivePickerComponent* Picker)override;

	virtual void Close()override;
};
