#pragma once

#include "S_ItemData.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SaveSubsystems/I_SaveableObject.h"
#include "AC_Container.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeightChanged, float, Weight);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoneyChanged, float, Money);

class UW_ItemSlot;

UCLASS(BlueprintType, Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSEXTRAS_API UAC_Container : public UActorComponent, public II_SaveableObject
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

    UPROPERTY(BlueprintAssignable) FOnWeightChanged OnWeightChanged;
    UPROPERTY(BlueprintAssignable) FOnMoneyChanged OnMoneyChanged;

    UFUNCTION(BlueprintCallable, Category = "ContainerInteraction")
    void AddToContainer(FName Name, int32 Quantity, float TradeCoeff, bool bShouldCount);

    UFUNCTION(BlueprintCallable, Category = "ContainerInteraction")
    void RemoveFromContainer(int32 ItemIndex, int32 Quantity, float TradeCoeff, bool bShouldCount, bool bShouldSpawn = false);

    UFUNCTION(BlueprintCallable, Category = "ContainerInteraction")
    bool SetItemQuantityByName(FName Name, int32 NewQuantity);

    /*
     * Найти предмет по имени.
     * @param Name         Имя предмета (FS_Item::Name).
     * @param OutItemIndex Индекс в Items или INDEX_NONE, если не найден.
     * @param OutQuantity  Количество в найденном стеке или 0, если не найден.
     * @return             true, если предмет найден.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Container")
    bool FindItemByName(FName Name, int32& OutItemIndex, int32& OutQuantity) const;

    UFUNCTION(BlueprintCallable, Category = "ContainerInteraction")
    bool SpawnRemovedItem(FName Name);

    UFUNCTION(BlueprintCallable, Category = "ContainerInteraction")
    bool SetItemMarking(UW_ItemSlot* SlotToBeMarked, EItemMarking NewMarking);

    UFUNCTION(BlueprintCallable, Category = "Sorting")
    void Items_Sort(EnumSortType SortType, bool bIsDecreasing);

    /** Контейнер принадлежит персонажу игрока? */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Container")
    bool IsPlayerContainer() const;

    /** ItemId (ProtectedItemId из FloorAssignmentComponent). Используется InventorySubsystem. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Container")
    FGuid GetActorItemId() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(VisibleDefaultsOnly, SaveGame, Category = "Summary")
    float TotalWeight = 0.f;

    UPROPERTY(EditDefaultsOnly, SaveGame, Category = "Summary")
    float TotalMoney = 0.f;

    UPROPERTY(SaveGame) TMap<FName, int32> ItemsToSpawn;
    UPROPERTY(SaveGame) FTimerHandle RemoveItemsHandle;

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Summary") float GetWeight();
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Summary") float GetMoney();
    UFUNCTION(BlueprintCallable, Category = "Summary") void SetWeight(float NewWeight);
    UFUNCTION(BlueprintCallable, Category = "Summary") void SetMoney(float NewValue);

    void CountContainerWeight();
    int32 GetItemIndexByTag(FGameplayTag ItemTag) const;
};