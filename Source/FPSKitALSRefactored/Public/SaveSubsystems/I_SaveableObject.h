#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Core/CubixonCoreData.h"
#include "ChoreSystem/ChoreEnums.h"
#include "I_SaveableObject.generated.h"

USTRUCT(BlueprintType)
struct FSaveItemData
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid ItemId;

	UPROPERTY()
	FGuid AttachingDropZoneId;

	UPROPERTY()
	FTransform ItemTransform;

	UPROPERTY()
	uint8 bIsHeld : 1{false};

	UPROPERTY()
	uint8 bIsAttached : 1{false};

	UPROPERTY()
	uint8 bHasSimulatedPhysics : 1{false};

	// Breakable actor
	UPROPERTY()
	uint8 bIsBroken : 1{false};
};

USTRUCT(BlueprintType)
struct FSaveChoreData
{
	GENERATED_BODY()

	UPROPERTY()
	EChoreStatus ChoreStatus;

	UPROPERTY()
	TArray<FSaveItemData> SaveItemData;
};

UINTERFACE(MinimalAPI, BlueprintType)
class UI_SaveableObject : public UInterface
{
	GENERATED_BODY()
};

class FPSKITALSREFACTORED_API II_SaveableObject
{
	GENERATED_BODY()

public:
	// Text file (and heirs) save data collection and application functions
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveObject")
	TMap<FString, FText> CollectTextFilesSaveData();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveObject")
	void ApplyTextFilesSaveData(const TMap<FString, FText>& SaveData);

	// Spreadsheet file save data collection and application functions
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveObject")
	TMap<FString, FSpreadsheetCells> CollectSpreadsheetSaveData();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveObject")
	void ApplySpreadsheetSaveData(const TMap<FString, FSpreadsheetCells>& SaveData);

	// File tree save data collection and application functions
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveObject")
	TMap<FString, FCubixonFileData> CollectFileTreeSaveData();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveObject")
	void ApplyFileTreeSaveData(const TMap<FString, FCubixonFileData>& SaveData);

	// CMail save data collection and application functions
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveObject")
	TMap<FString, FCubixonCMailConversationsWrap> CollectCMailSaveData();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveObject")
	void ApplyCMailSaveData(const TMap<FString, FCubixonCMailConversationsWrap>& SaveData);

	// Backup save data collection and application functions
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveObject")
	FGlobalBackupData CollectBackupSaveData();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveObject")
	void ApplyBackupSaveData(const FGlobalBackupData& SaveData);

	// ChorePlacement save data collection and application functions
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveObject")
	FSaveChoreData CollectChorePlacementSaveData();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveObject")
	void ApplyChorePlacementSaveData(const FSaveChoreData& SaveDataArray);
};
