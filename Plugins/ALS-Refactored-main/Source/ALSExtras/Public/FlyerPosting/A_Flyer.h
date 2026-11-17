#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "A_Flyer.generated.h"

UCLASS()
class ALSEXTRAS_API AA_Flyer : public AActor
{
	GENERATED_BODY()
	
public:	
	AA_Flyer();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

public:	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flyer")
	UStaticMeshComponent* FlyerMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Flyer")
	UDataTable* InventoryItemsDataTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flyer")
	FGuid FlyerId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Flyer")
	FGameplayTag FlyerTag;

private:
	UPROPERTY()
	UMaterialInstanceDynamic* MID_Flyer;
};
