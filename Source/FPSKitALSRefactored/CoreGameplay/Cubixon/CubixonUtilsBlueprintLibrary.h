#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Fonts/SlateFontInfo.h"
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
};