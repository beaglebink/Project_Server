#include "CubixonUtilsBlueprintLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include <Blueprint/WidgetLayoutLibrary.h>

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

float UCubixonUtilsBlueprintLibrary::EstimateMultilineTextHeight(const FString& Text, const FSlateFontInfo& FontInfo, float WrapWidth)
{
    if (Text.IsEmpty())
    {
        return 0.f;
    }

    TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
    FText FormattedText = FText::FromString(Text);

    // Получаем высоту в Slate Units
    FVector2D MeasuredSize = FontMeasure->Measure(FormattedText, FontInfo, WrapWidth);

    // Преобразуем в пиксели с учётом DPI
    float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(GWorld->GetFirstPlayerController());
    float HeightInPixels = MeasuredSize.Y / ViewportScale;

    return HeightInPixels;

}

int32 UCubixonUtilsBlueprintLibrary::CountWrappedLines(const FString& Text, const FSlateFontInfo& FontInfo, float WrapWidth)  
{  
   FString TrimmedText = Text.TrimStartAndEnd(); // Удаляем лидирующие и конечные пробелы  

   TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();  

   TArray<FString> Words;  
   TrimmedText.ParseIntoArray(Words, TEXT(" "), true);  

   FString CurrentLine;  
   int32 LineCount = 1;  

   for (const FString& Word : Words)  
   {  
       FString TestLine = CurrentLine.IsEmpty() ? Word : CurrentLine + TEXT(" ") + Word;  
       FVector2D Size = FontMeasure->Measure(TestLine, FontInfo, WrapWidth);  

       if (Size.X > WrapWidth)  
       {  
           LineCount++;  
           CurrentLine = Word;  
       }  
       else  
       {  
           CurrentLine = TestLine;  
       }  
   }  

   return LineCount;  
}

FText UCubixonUtilsBlueprintLibrary::RemoveLineBreaks(const FText& Input)
{
    FString S = Input.ToString();
    S.ReplaceInline(TEXT("\r\n"), TEXT(" "));
    S.ReplaceInline(TEXT("\n"), TEXT(" "));
    S.ReplaceInline(TEXT("\r"), TEXT(" "));
    return FText::FromString(S);
}





