#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ISaveableSubsystem.h"
#include "GIS_Container.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UGIS_Container : public UGameInstanceSubsystem, public ISaveableSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	//SaveableSubsystem interface
	virtual void CollectSaveData(FSubsystemSaveData& OutData) override;

	virtual void ApplySaveData(const FSubsystemSaveData& InData) override;

	void ClearTransientData_Implementation();

	virtual FString GetSaveSubsystemName() const override { return TEXT("ContainerSubsystem"); }

	virtual bool GetIsLoadComplete() const override { return bIsLoadComplete; }

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveableObject")
	TArray<UObject*> SaveableObjects;

	UFUNCTION(BlueprintCallable, Category = "SaveableObject")
	void AddSaveableObject(UObject* NewSaveableObject);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveableObject")
	void ApplyContainerData(UActorComponent* ContainerComponent);

private:
	TMap<FGuid, int32> CachedContainerData;

	bool bIsLoadComplete = false;
};
