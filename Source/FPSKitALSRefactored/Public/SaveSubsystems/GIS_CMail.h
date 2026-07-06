#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ISaveableSubsystem.h"
#include "Core/CubixonCoreData.h"
#include "SaveSubsystems/I_SaveableObject.h"
#include "GIS_CMail.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UGIS_CMail : public UGameInstanceSubsystem, public ISaveableSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	//SaveableSubsystem interface
	virtual void CollectSaveData(FSubsystemSaveData& OutData) override;

	virtual void ApplySaveData(const FSubsystemSaveData& InData) override;

	void ClearTransientData_Implementation();

	virtual FString GetSaveSubsystemName() const override { return TEXT("CMailSubsystem"); }

	virtual bool GetIsLoadComplete() const override { return bIsLoadComplete; }

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveableObject")
	TArray<UObject*> SaveableObjects;

	UFUNCTION(BlueprintCallable, Category = "SaveableObject")
	void AddSaveableObject(UObject* NewSaveableObject);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CMail")
	void ApplyCMailData(UObject* ProfileObject);

private:
	TMap<FString, FCubixonCMailConversationsWrap> CachedCMailData;

	bool bIsLoadComplete = false;
};
