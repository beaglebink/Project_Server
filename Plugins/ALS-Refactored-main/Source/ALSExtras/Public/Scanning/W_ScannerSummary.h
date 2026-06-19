#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_ScannerSummary.generated.h"

UCLASS()
class ALSEXTRAS_API UW_ScannerSummary : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Scanning")
	void RefreshSummary(const TArray<AActor*>& ScannedActors);
};
