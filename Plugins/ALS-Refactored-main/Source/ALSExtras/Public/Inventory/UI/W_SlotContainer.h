#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_SlotContainer.generated.h"

UCLASS()
class ALSEXTRAS_API UW_SlotContainer : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
};
