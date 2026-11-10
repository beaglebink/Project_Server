#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ISaveableSubsystem.h"
#include "GIS_RealEstatePaperWork.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UGIS_RealEstatePaperWork : public UGameInstanceSubsystem, public ISaveableSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	//SaveableSubsystem interface
	virtual void CollectSaveData(FSubsystemSaveData& OutData) override;

	virtual void ApplySaveData(const FSubsystemSaveData& InData) override;

	void ClearTransientData_Implementation();

	virtual FString GetSaveSubsystemName() const override { return TEXT("RealEstatePaperWorkSubsystem"); }

	virtual bool GetIsLoadComplete() const override { return bIsLoadComplete; }

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveableObject")
	TArray<UObject*> SaveableObjects;

	UFUNCTION(BlueprintCallable, Category = "SaveableObject")
	void AddSaveableObject(UObject* NewSaveableObject);

	TMap<FName, int32> CachedQuestData;

private:
	bool bIsLoadComplete = false;
};
