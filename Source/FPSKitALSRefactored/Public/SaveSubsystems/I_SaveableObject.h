#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Core/CubixonCoreData.h"
#include "I_SaveableObject.generated.h"

USTRUCT(BlueprintType)
struct FSpreadsheetCells
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpreadSheet")
	TMap<FIntPoint, FText> Cells;
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
};
