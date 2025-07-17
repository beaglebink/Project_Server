#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_VisualDescription.generated.h"

class AA_3DDescription;
class UDataTable;
class UTextBlock;

UCLASS()
class ALSEXTRAS_API UW_VisualDescription : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, Category = "Slot text field", meta = (BindWidget))
	UTextBlock* TextBlock_Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items")
	UDataTable* ItemDataTable;

	UPROPERTY(BlueprintReadOnly, Category = "Items")
	FName ItemName;

protected:
	UPROPERTY(BlueprintReadWrite, Category = "Render")
	AA_3DDescription* RenderActor;

	UPROPERTY(EditDefaultsOnly, Category = "RenderActor")
	TSubclassOf<AA_3DDescription> RenderActorClass;

	virtual void NativeConstruct() override;
	
	virtual void NativeDestruct() override;
};
