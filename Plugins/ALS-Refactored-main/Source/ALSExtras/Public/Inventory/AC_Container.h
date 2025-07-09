#pragma once

#include "S_ItemData.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AC_Container.generated.h"


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

	UFUNCTION(BlueprintCallable, Category = "ContainerInteraction")
	void AddToContainer(FName Name, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "ContainerInteraction")
	void RemoveFromContainer(FName Name, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void A_Z_Sort(bool bIsDecreasing);

	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void Damage_Sort(bool bIsDecreasing);

	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void Armor_Sort(bool bIsDecreasing);

	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void Durability_Sort(bool bIsDecreasing);

	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void Weight_Sort(bool bIsDecreasing);

	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void Value_Sort(bool bIsDecreasing);
};
