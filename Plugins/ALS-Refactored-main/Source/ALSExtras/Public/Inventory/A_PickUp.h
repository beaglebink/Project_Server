#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "S_ItemData.h"
#include "A_PickUp.generated.h"

UCLASS(BlueprintType, Blueprintable)
class ALSEXTRAS_API AA_PickUp : public AActor
{
	GENERATED_BODY()
	
public:	
	AA_PickUp();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	UDataTable* ItemDataTable;

	UFUNCTION(BlueprintCallable, Category = "Item")
	const FS_ItemData GetItemData() const;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
