#include "CubixonUtilsBlueprintLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"

FString UCubixonUtilsBlueprintLibrary::TruncateTextWithEllipsis(const FString& InputText, const FSlateFontInfo& FontInfo, float MaxWidth, float FontScale)
{
    if (InputText.IsEmpty())
    {
        return FString();
    }

    TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

    const FString Ellipsis = TEXT("...");
    const FVector2D EllipsisSize = FontMeasure->Measure(Ellipsis, FontInfo);

    FString Result;
    for (int32 i = 0; i < InputText.Len(); ++i)
    {
        FString TestString = InputText.Left(i + 1) + Ellipsis;
        FVector2D TestSize = FontMeasure->Measure(TestString, FontInfo, FontScale);

        if (TestSize.X > MaxWidth)
        {
            Result = InputText.Left(i) + Ellipsis;
            return Result;
        }
    }

    return InputText;
}

FVector2D UCubixonUtilsBlueprintLibrary::GetTextWidth(const FString& InputText, const FSlateFontInfo& FontInfo, float FontScale)
{  
   if (InputText.IsEmpty())  
   {  
       return FVector2D::Zero();  
   }  

   TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();  
   FVector2D TextSize = FontMeasure->Measure(InputText, FontInfo, FontScale);  

   return TextSize;  
}

float UCubixonUtilsBlueprintLibrary::MeasureMultilineTextHeight(const FString& Text, const FSlateFontInfo& FontInfo, float WrapWidth)
{
    if (Text.IsEmpty())
    {
        return 0.f;
    }

    TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    FText FormattedText = FText::FromString(Text);

    // Measure with wrapping
    FVector2D MeasuredSize = FontMeasure->Measure(FormattedText, FontInfo, WrapWidth);

    return MeasuredSize.Y;
}


