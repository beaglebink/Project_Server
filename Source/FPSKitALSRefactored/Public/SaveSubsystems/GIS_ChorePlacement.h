#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ISaveableSubsystem.h"
#include "ItemPlacement/A_ChorePlacementZone.h"
#include "GIS_ChorePlacement.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UGIS_ChorePlacement : public UGameInstanceSubsystem, public ISaveableSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	//SaveableSubsystem interface
	virtual void CollectSaveData(FSubsystemSaveData& OutData) override;

	virtual void ApplySaveData(const FSubsystemSaveData& InData) override;

	void ClearTransientData_Implementation();

	virtual FString GetSaveSubsystemName() const override { return TEXT("ChorePlacementSubsystem"); }

	virtual bool GetIsLoadComplete() const override { return bIsLoadComplete; }

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveableObject")
	TArray<UObject*> SaveableObjects;

	UFUNCTION(BlueprintCallable, Category = "SaveableObject")
	void AddSaveableObject(UObject* NewSaveableObject);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveableObject")
	void ApplyChorePlacementData(AActor* ChorePlacementZone);

private:
	TMap<FGuid, FSaveChoreData> CachedChorePlacementZoneData;

	bool bIsLoadComplete = false;
};
