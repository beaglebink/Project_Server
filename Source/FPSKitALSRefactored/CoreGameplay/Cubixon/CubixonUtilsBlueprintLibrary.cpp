#include "CubixonUtilsBlueprintLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "Blueprint/UserWidget.h"


#include "Blueprint/WidgetLayoutLibrary.h"

#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Components/ContentWidget.h"


#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/World.h"
#include "Slate/WidgetTransform.h"
#include <Components/WidgetSwitcher.h>

#include "Math/UnrealMathUtility.h"

#include "Serialization/ObjectAndNameAsStringProxyArchive.h"


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

	if (RealLines.Num() == 0)
		return;

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

bool UCubixonUtilsBlueprintLibrary::IsScreenPositionOverWidget(UWidget* Widget, const FVector2D& ScreenPosition)
{
	if (!Widget || !Widget->IsVisible())
	{
		return false;
	}

	FGeometry WidgetGeometry = Widget->GetCachedGeometry();

	return WidgetGeometry.IsUnderLocation(ScreenPosition);
}

FString UCubixonUtilsBlueprintLibrary::GenerateRandomHex64(int32 HighMin, int32 HighMax, int32 LowMin, int32 LowMax)
{
	// Генерация случайных чисел в заданных диапазонах
	uint64 High = static_cast<uint64>(FMath::RandRange(HighMin, HighMax)) << 32;
	uint64 Low = static_cast<uint64>(FMath::RandRange(LowMin, LowMax));

	// Объединение в одно 64-битное число
	uint64 Random64 = High | Low;

	// Форматирование в строку с ведущими нулями
	return FString::Printf(TEXT("0x%016llX"), Random64);
}

FString UCubixonUtilsBlueprintLibrary::IntToChar(int32 Value)
{
	if (Value >= 0 && Value <= 255)
	{
		FString Result;
		Result.AppendChar(static_cast<TCHAR>(Value));
		return Result;
	}
	return FString();
}

bool UCubixonUtilsBlueprintLibrary::IsPointInsideWidget(UWidget* Widget, const FVector2D& ScreenPosition)
{
	if (!Widget) return false;

	const FGeometry& Geometry = Widget->GetCachedGeometry();
	const FVector2D AbsPos = Geometry.GetAbsolutePosition();
	const FVector2D Size = Geometry.GetAbsoluteSize();

	FSlateRect Rect(AbsPos.X, AbsPos.Y, AbsPos.X + Size.X, AbsPos.Y + Size.Y);
	return Rect.ContainsPoint(ScreenPosition);
}

// Рекурсивный обход дерева виджетов
void UCubixonUtilsBlueprintLibrary::CollectWidgetsAtPoint(UWidget* Widget, const FVector2D& ScreenPosition, TArray<UWidget*>& OutWidgets)
{
	if (!Widget) return;

	// Проверяем реальную видимость (учитывает родителей)
	if (!Widget->IsVisible())
	{
		return;
	}

	if (IsPointInsideWidget(Widget, ScreenPosition))
	{
		OutWidgets.Add(Widget);
	}

	// Обход детей
	if (UWidgetSwitcher* Switcher = Cast<UWidgetSwitcher>(Widget))
	{
		// Берём только активный виджет
		if (UWidget* Active = Switcher->GetActiveWidget())
		{
			CollectWidgetsAtPoint(Active, ScreenPosition, OutWidgets);
		}
	}
	else if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
	{
		const int32 Count = Panel->GetChildrenCount();
		for (int32 i = 0; i < Count; ++i)
		{
			CollectWidgetsAtPoint(Panel->GetChildAt(i), ScreenPosition, OutWidgets);
		}
	}
	else if (UContentWidget* ContentWidget = Cast<UContentWidget>(Widget))
	{
		if (UWidget* Child = ContentWidget->GetContent())
		{
			CollectWidgetsAtPoint(Child, ScreenPosition, OutWidgets);
		}
	}
	else if (UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
	{
		if (UWidget* ChildRoot = UserWidget->GetRootWidget())
		{
			CollectWidgetsAtPoint(ChildRoot, ScreenPosition, OutWidgets);
		}
	}
}
// Главная функция: получить список виджетов под экранной точкой
TArray<UWidget*> UCubixonUtilsBlueprintLibrary::GetWidgetsAtScreenPosition(UUserWidget* RootUserWidget, const FVector2D& ScreenPosition)
{
	TArray<UWidget*> Result;

	if (RootUserWidget)
	{
		if (UWidget* Root = RootUserWidget->GetRootWidget())
		{
			CollectWidgetsAtPoint(Root, ScreenPosition, Result);
		}
	}

	return Result;
}

FCubixonFileData UCubixonUtilsBlueprintLibrary::CreateCubixonFileData(FText FileName, USceneComponent* CubixonFile)
{
	FCubixonFileData Result;

	if (!CubixonFile)
	{
		return Result;
	}

	Result.FileName = FileName;

	Result.ComponentClass = CubixonFile->GetClass();

	FMemoryWriter Writer(Result.SerializedData, true);

	FObjectAndNameAsStringProxyArchive Ar(Writer, true);
	Ar.ArIsSaveGame = true;

	CubixonFile->Serialize(Ar);

	return Result;
}

USceneComponent* UCubixonUtilsBlueprintLibrary::CreateCubixonFileFromData(const FCubixonFileData& CubixonFileData, UObject* Outer, USceneComponent* ParentComponent)
{

	if (!Outer || !CubixonFileData.ComponentClass)
	{
		return nullptr;
	}

	USceneComponent* NewComponent = NewObject<USceneComponent>(Outer, CubixonFileData.ComponentClass, *CubixonFileData.FileName.ToString());

	if (!NewComponent)
	{
		return nullptr;
	}

	FMemoryReader Reader(CubixonFileData.SerializedData, true);

	FObjectAndNameAsStringProxyArchive Ar(Reader, true);
	Ar.ArIsSaveGame = true;

	NewComponent->Serialize(Ar);

	NewComponent->RegisterComponent();

	if (AActor* OwnerActor = Cast<AActor>(Outer))
	{
		OwnerActor->AddInstanceComponent(NewComponent);

		if (OwnerActor->GetRootComponent())
		{
			NewComponent->AttachToComponent(ParentComponent, FAttachmentTransformRules::KeepRelativeTransform);
		}
	}

	return NewComponent;
}

bool UCubixonUtilsBlueprintLibrary::IsChildActor(const AActor* Actor)
{
	if (!Actor) return false;

	// Проверка на прикреплённого актора
	if (Actor->GetAttachParentActor() != nullptr)
		return true;

	// Проверка на актора, созданного через ChildActorComponent
	AActor* Owner = Actor->GetOwner();
	if (Owner)
	{
		TArray<UChildActorComponent*> ChildComps;
		Owner->GetComponents<UChildActorComponent>(ChildComps);
		for (UChildActorComponent* Comp : ChildComps)
		{
			if (Comp->GetChildActor() == Actor)
				return true;
		}
	}
	return false;
}