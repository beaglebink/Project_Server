#pragma once

#include "CoreMinimal.h"
#include "Inventory/A_BaseInteractiveContainer.h"
#include "A_Vendor.generated.h"

UCLASS()
class ALSEXTRAS_API AA_Vendor : public AA_BaseInteractiveContainer
{
	GENERATED_BODY()
	
public:	
	AA_Vendor();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Summary")
	float MoneyAmount;

protected:

	virtual void TimelineProgress(float Value) override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void Open(UInteractivePickerComponent* Picker)override;

	virtual void Close()override;
};
