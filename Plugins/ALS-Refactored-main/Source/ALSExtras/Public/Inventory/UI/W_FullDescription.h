#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_FullDescription.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class ALSEXTRAS_API UW_FullDescription : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativePreConstruct() override;

public:
	UPROPERTY(BlueprintReadWrite, Category = "Name", meta = (ExposeOnSpawn = "true"))
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Items")
	UDataTable* ItemDataTable;

	UPROPERTY(BlueprintReadWrite, Category = "Slot Image field", meta = (BindWidget))
	UImage* Image_Description;

	UPROPERTY(BlueprintReadWrite, Category = "Slot text field", meta = (BindWidget))
	UTextBlock* TextBlock_Description;
};
