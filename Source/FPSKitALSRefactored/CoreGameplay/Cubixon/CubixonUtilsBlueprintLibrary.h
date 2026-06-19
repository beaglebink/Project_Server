#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Fonts/SlateFontInfo.h"
#include <Components/MultiLineEditableText.h>
#include <Components/EditableText.h>
#include <Components/VerticalBox.h>
#include "Core/CubixonCoreData.h"
#include "CubixonUtilsBlueprintLibrary.generated.h"

class IFontMeasure;

UCLASS()
class FPSKITALSREFACTORED_API UCubixonUtilsBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Cubixon|Text")
	static FString TruncateTextWithEllipsis(const FString& InputText, const FSlateFontInfo& FontInfo, float MaxWidth, float FontScale = 1.0);

	UFUNCTION(BlueprintCallable, Category = "Cubixon|Text")
	static FVector2D GetTextWidth(const FString& InputText, const FSlateFontInfo& FontInfo, float FontScale = 1.0);

	UFUNCTION(BlueprintCallable, Category = "Cubixon|Text")
	static float MeasureMultilineTextHeight(const FString& Text, const FSlateFontInfo& FontInfo, float WrapWidth);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cubixon|Text")
	static FText RemoveLineBreaks(const FText& Input);

	UFUNCTION(BlueprintCallable, Category = "Cubixon|Text")
	static void SetCursorAtClickForEditableText(UMultiLineEditableText* EditableText, const FVector2D& ScreenClickPosition);

	UFUNCTION(BlueprintCallable, Category = "Cubixon|Text")
	static FString TrimTextToFitWidth(const FString& Text, const FSlateFontInfo& FontInfo, float MaxWidth);

	UFUNCTION(BlueprintCallable, Category = "UI")
	static void InsertWidgetAt(UVerticalBox* Box, UWidget* WidgetToAdd, int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Widgets")
	static bool IsScreenPositionOverWidget(UWidget* Widget, const FVector2D& ScreenPosition);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Random")
	static FString GenerateRandomHex64(int32 HighMin, int32 HighMax, int32 LowMin, int32 LowMax);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Utility")
	static FString IntToChar(int32 Value);

public:
	UFUNCTION(BlueprintCallable, Category = "Widgets")
	static TArray<UWidget*> GetWidgetsAtScreenPosition(UUserWidget* RootUserWidget, const FVector2D& ScreenPosition);

private:
	static bool IsPointInsideWidget(UWidget* Widget, const FVector2D& ScreenPosition);
	static void CollectWidgetsAtPoint(UWidget* Widget, const FVector2D& ScreenPosition, TArray<UWidget*>& OutWidgets);

public:
	UFUNCTION(BlueprintCallable, Category = "CubixonFile")
	static FCubixonFileData CreateCubixonFileData(FText FileName, USceneComponent* CubixonFile);

	UFUNCTION(BlueprintCallable, Category = "CubixonFile")
	static USceneComponent* CreateCubixonFileFromData(const FCubixonFileData& CubixonFileData, UObject* Outer, USceneComponent* ParentComponent);
};


