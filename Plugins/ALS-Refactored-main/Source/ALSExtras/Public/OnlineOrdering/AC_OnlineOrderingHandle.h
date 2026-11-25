#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OnlineOrdering/PDA_OnlineOrdering.h"
#include "AC_OnlineOrderingHandle.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALSEXTRAS_API UAC_OnlineOrderingHandle : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAC_OnlineOrderingHandle();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OnlineOrderingData")
	TMap<FName, FOnlineOrderingData> OnlineOrderingDataMap;

	UFUNCTION(BlueprintCallable, Category = "OnlineOrdering")
	bool LoadOnlineOrderingData(FName OrderName);

	UFUNCTION(BlueprintCallable, Category = "ChoreStatus")
	void SetChoreStatus(FName OrderName, EChoreStatus NewStatus);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ChoreStatus")
	EChoreStatus GetChoreStatus(FName OrderName)const;

	// Data saving
	UFUNCTION(BlueprintCallable, Category = "Saving")
	FString CollectSaveData();

	UFUNCTION(BlueprintCallable, Category = "Saving")
	void ApplySaveData(FString Jsonstring);

	UFUNCTION(BlueprintCallable, Category = "OnlineOrdering")
	bool CheckIfOrderIsCompleted(FName OrderName);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "OnlineOrdering")
	FOnlineOrderingData GetActiveOrder();

};
