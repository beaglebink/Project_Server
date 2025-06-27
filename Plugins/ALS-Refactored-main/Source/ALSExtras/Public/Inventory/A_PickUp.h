#pragma once

#include "S_ItemData.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_PickUp.generated.h"

class UInteractiveItemComponent;

UCLASS(BlueprintType, Blueprintable)
class ALSEXTRAS_API AA_PickUp : public AActor
{
	GENERATED_BODY()

public:
	AA_PickUp();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* StaticMeshComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UInteractiveItemComponent* InteractiveComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FS_Item Item;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	UDataTable* ItemDataTable;

	UFUNCTION(BlueprintCallable, Category = "Item")
	FS_ItemData GetItemData() const;

protected:
	virtual void BeginPlay() override;

};
