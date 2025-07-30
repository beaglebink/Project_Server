#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_CharacterUI.generated.h"

UCLASS()
class ALSEXTRAS_API UW_CharacterUI : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnInitialized() override;
};
