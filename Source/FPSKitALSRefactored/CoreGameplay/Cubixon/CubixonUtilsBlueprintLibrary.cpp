#include "CubixonUtilsBlueprintLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Widgets/Text/SMultiLineEditableText.h"
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

FText UCubixonUtilsBlueprintLibrary::RemoveLineBreaks(const FText& Input)
{
    FString S = Input.ToString();
    S.ReplaceInline(TEXT("\r\n"), TEXT(" "));
    S.ReplaceInline(TEXT("\n"), TEXT(" "));
    S.ReplaceInline(TEXT("\r"), TEXT(" "));
    return FText::FromString(S);
}

void UCubixonUtilsBlueprintLibrary::SetCursorAtClickForEditableText(UMultiLineEditableText* EditableText, const FVector2D& ScreenClickPosition)
{
    if (!EditableText) return;

    TSharedPtr<SWidget> WidgetPtr = EditableText->TakeWidget();
    TSharedPtr<SMultiLineEditableText> SlateWidget = StaticCastSharedPtr<SMultiLineEditableText>(WidgetPtr);
    if (!SlateWidget.IsValid()) return;

    const FGeometry& Geometry = SlateWidget->GetCachedGeometry();
    const FVector2D LocalClickPosition = Geometry.AbsoluteToLocal(ScreenClickPosition);

    const FSlateFontInfo FontInfo = SlateWidget->GetFont();
    const FString FullText = SlateWidget->GetText().ToString();
    const float WrapWidth = EditableText->GetWrapTextAt() > 0.0f ? EditableText->GetWrapTextAt() : Geometry.GetLocalSize().X;

    TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

    TArray<FString> RealLines;
    FullText.ParseIntoArrayLines(RealLines);

    struct FVisualLine
    {
        FString Text;
        int32 RealLineIndex;
        int32 CharOffsetInRealLine;
    };

    TArray<FVisualLine> VisualLines;

    int32 GlobalOffset = 0;

    for (int32 RealLineIndex = 0; RealLineIndex < RealLines.Num(); ++RealLineIndex)
    {
        const FString& RealLine = RealLines[RealLineIndex];

        TArray<FString> Tokens;
        FString Token;
        for (int32 c = 0; c < RealLine.Len(); ++c)
        {
            TCHAR Ch = RealLine[c];
            if (Ch == ' ')
            {
                if (!Token.IsEmpty())
                {
                    Tokens.Add(Token);
                    Token.Empty();
                }
                Tokens.Add(TEXT(" "));
            }
            else
            {
                Token.AppendChar(Ch);
            }
        }
        if (!Token.IsEmpty())
        {
            Tokens.Add(Token);
        }

        FString CurrentLine;
        float CurrentLineWidth = 0.0f;
        int32 CharOffsetInRealLine = 0;

        for (const FString& T : Tokens)
        {
            float TokenWidth = FontMeasure->Measure(T, FontInfo).X;

            if (CurrentLineWidth + TokenWidth > WrapWidth && !CurrentLine.IsEmpty())
            {
                VisualLines.Add({ CurrentLine, RealLineIndex, CharOffsetInRealLine });
                CharOffsetInRealLine += CurrentLine.Len();
                CurrentLine = T;
                CurrentLineWidth = TokenWidth;
            }
            else
            {
                CurrentLine += T;
                CurrentLineWidth += TokenWidth;
            }
        }

        if (!CurrentLine.IsEmpty())
        {
            VisualLines.Add({ CurrentLine, RealLineIndex, CharOffsetInRealLine });
        }

        GlobalOffset += RealLine.Len() + 1; // +1 за \n
    }

    float Y = 0.0f;
    for (const FVisualLine& Line : VisualLines)
    {
        const FVector2D LineSize = FontMeasure->Measure(Line.Text, FontInfo);

        if (LocalClickPosition.Y < Y + LineSize.Y)
        {
            float AccumulatedX = 0.0f;
            int32 CharIndexInLine = 0;

            for (int32 j = 0; j < Line.Text.Len(); ++j)
            {
                const FString CharStr = Line.Text.Mid(j, 1);
                const float CharWidth = FontMeasure->Measure(CharStr, FontInfo).X;

                if (LocalClickPosition.X < AccumulatedX + CharWidth / 2.0f)
                {
                    CharIndexInLine = j;
                    break;
                }

                AccumulatedX += CharWidth;
            }

            if (CharIndexInLine == 0 && LocalClickPosition.X >= AccumulatedX)
            {
                CharIndexInLine = Line.Text.Len();
            }

            const int32 CharIndexInRealLine = Line.CharOffsetInRealLine + CharIndexInLine;
            SlateWidget->GoTo(FTextLocation(Line.RealLineIndex, CharIndexInRealLine));
            return;
        }

        Y += LineSize.Y;
    }

    SlateWidget->GoTo(FTextLocation(RealLines.Num() - 1, RealLines.Last().Len()));
}

FString UCubixonUtilsBlueprintLibrary::TrimTextToFitWidth(const FString& Text, const FSlateFontInfo& FontInfo, float MaxWidth)
{
    TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

    float AccumulatedWidth = 0.0f;
    int32 MaxChars = Text.Len();

    for (int32 i = 0; i < Text.Len(); ++i)
    {
        const FString CharStr = Text.Mid(i, 1);
        const float CharWidth = FontMeasure->Measure(CharStr, FontInfo).X;

        if (AccumulatedWidth + CharWidth > MaxWidth)
        {
            MaxChars = i;
            break;
        }

        AccumulatedWidth += CharWidth;
    }

    return Text.Left(MaxChars);
}

void UCubixonUtilsBlueprintLibrary::InsertWidgetAt(UVerticalBox* Box, UWidget* WidgetToAdd, int32 Index)
{
    /*
    if (!Box || !WidgetToAdd) return;

    TArray<UWidget*> ExistingChildren;
    const int32 Count = Box->GetChildrenCount();

    // Сохраняем текущих детей
    for (int32 i = 0; i < Count; ++i)
    {
        ExistingChildren.Add(Box->GetChildAt(i));
    }

    // Очищаем
    Box->ClearChildren();

    // Вставляем в нужную позицию
    Index = FMath::Clamp(Index, 0, ExistingChildren.Num());
    ExistingChildren.Insert(WidgetToAdd, Index);

    // Добавляем всех обратно
    for (UWidget* Child : ExistingChildren)
    {
        Box->AddChild(Child);
    }
    */


    if (!Box || !WidgetToAdd) return;

    UPanelSlot* NewSlot = Box->AddChild(WidgetToAdd);
    if (!NewSlot) return;

    TArray<UPanelSlot*>& Slots = const_cast<TArray<UPanelSlot*>&>(Box->GetSlots());

    Slots.Remove(NewSlot);
    Index = FMath::Clamp(Index, 0, Slots.Num());
    Slots.Insert(NewSlot, Index);

    Box->InvalidateLayoutAndVolatility();

}

