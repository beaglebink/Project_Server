#pragma once

#include "S_ItemData.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AC_Container.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeightChanged, float, Weight);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoneyChanged, float, Money);

class UW_ItemSlot;

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSEXTRAS_API UAC_Container : public UActorComponent
{
	GENERATED_BODY()

public:
	UAC_Container();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, SaveGame, Category = "Items")
	TArray<FS_Item> Items;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Items")
	UDataTable* ItemDataTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Item")
	USoundBase* SpawnSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Item")
	USoundBase* PickUpSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, SaveGame, Category = "Trading")
	float TradeCoefficient = 1.0f;

	UPROPERTY(BlueprintAssignable)
	FOnWeightChanged OnWeightChanged;

	UPROPERTY(BlueprintAssignable)
	FOnMoneyChanged OnMoneyChanged;

	UFUNCTION(BlueprintCallable, Category = "ContainerInteraction")
	void AddToContainer(FName Name, int32 Quantity, float TradeCoeff, bool bShouldCount);

	UFUNCTION(BlueprintCallable, Category = "ContainerInteraction")
	void RemoveFromContainer(int32 ItemIndex, int32 Quantity, float TradeCoeff, bool bShouldCount, bool bShouldSpawn = false);

	UFUNCTION(BlueprintCallable, Category = "ContainerInteraction")
	bool SpawnRemovedItem(FName Name);

	UFUNCTION(BlueprintCallable, Category = "ContainerInteraction")
	bool SetItemMarking(UW_ItemSlot* SlotToBeMarked, EItemMarking NewMarking);

	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void Items_Sort(EnumSortType SortType, bool bIsDecreasing);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleDefaultsOnly, SaveGame, Category = "Summary")
	float TotalWeight;

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "Summary")
	float TotalMoney;

	UPROPERTY(SaveGame)
	TMap<FName, int32> ItemsToSpawn;

	UPROPERTY(SaveGame)
	FTimerHandle RemoveItemsHandle;

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Summary")
	float GetWeight();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Summary")
	float GetMoney();

	UFUNCTION(BlueprintCallable, Category = "Summary")
	void SetWeight(float NewWeight);

	UFUNCTION(BlueprintCallable, Category = "Summary")
	void SetMoney(float NewValue);

	void CountContainerWeight();
};
