#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Fonts/SlateFontInfo.h"
#include <Components/MultiLineEditableText.h>
#include <Components/EditableText.h>
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
};


