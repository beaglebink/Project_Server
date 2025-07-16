#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_VisualDescription.generated.h"

UCLASS()
class ALSEXTRAS_API UW_VisualDescription : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
};
