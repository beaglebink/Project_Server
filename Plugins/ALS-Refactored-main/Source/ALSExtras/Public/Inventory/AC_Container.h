#pragma once

#include "S_ItemData.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AC_Container.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArmourChanged, float, Armour);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeightChanged, float, Weight);

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSEXTRAS_API UAC_Container : public UActorComponent
{
	GENERATED_BODY()

public:
	UAC_Container();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Items")
	TArray<FS_Item> Items;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items")
	UDataTable* ItemDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	USoundBase* SpawnSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	USoundBase* PickUpSound;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Summary")
	float TotalArmour;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Summary")
	float TotalWeight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Summary")
	float TotalMoney;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Trading")
	float TradeCoefficient = 1.0f;

	UPROPERTY(BlueprintAssignable)
	FOnArmourChanged OnArmourChanged;

	UPROPERTY(BlueprintAssignable)
	FOnWeightChanged OnWeightChanged;

	UFUNCTION(BlueprintCallable, Category = "ContainerInteraction")
	void AddToContainer(FName Name, int32 Quantity, float TradeCoeff, bool bShouldCount);

	UFUNCTION(BlueprintCallable, Category = "ContainerInteraction")
	void RemoveFromContainer(FName Name, int32 Quantity, float TradeCoeff, bool bShouldCount, bool bShouldSpawn = false);

	UFUNCTION(BlueprintCallable, Category = "ContainerInteraction")
	bool SpawnRemovedItem(FName Name);

	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void Items_Sort(EnumSortType SortType, bool bIsDecreasing);

private:
	TMap<FName, int32> ItemsToSpawn;

	FTimerHandle RemoveItemsHandle;
};
