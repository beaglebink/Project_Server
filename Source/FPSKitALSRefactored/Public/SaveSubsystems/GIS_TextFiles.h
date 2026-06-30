#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ISaveableSubsystem.h"
#include "SaveSubsystems/I_SaveableObject.h"
#include "GIS_TextFiles.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UGIS_TextFiles : public UGameInstanceSubsystem, public ISaveableSubsystem
{
	GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    virtual void Deinitialize() override;

	//SaveableSubsystem interface
    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    
    virtual FString GetSaveSubsystemName() const override { return TEXT("TextFilesSubsystem"); }
    
    virtual bool GetIsLoadComplete() const override { return bIsLoadComplete; }

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "SaveableObject")
    UObject* SaveableObject;

private:
    bool bIsLoadComplete = false;
};
