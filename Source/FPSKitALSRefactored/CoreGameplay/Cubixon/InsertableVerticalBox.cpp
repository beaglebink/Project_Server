#include "InsertableVerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UInsertableVerticalBox::InitializeWidgetsArray()
{
    if (Widgets.Num() == 0 && GetChildrenCount() > 0)
    {
        for (int32 i = 0; i < GetChildrenCount(); i++)
        {
            Widgets.Add(GetChildAt(i));
        }
    }
}

void UInsertableVerticalBox::InsertWidgetAt(UWidget* NewWidget, int32 Index)
{
    if (!NewWidget) return;

    // Инициализируем массив, если он ещё пуст
    InitializeWidgetsArray();

    // Если виджет ещё не в массиве — добавляем
    if (!Widgets.Contains(NewWidget))
    {
        Index = FMath::Clamp(Index, 0, Widgets.Num());
        Widgets.Insert(NewWidget, Index);
    }

    // Отключаем от панели все виджеты ниже позиции вставки
    for (int32 i = Index; i < GetChildrenCount(); i++)
    {
        UWidget* Child = GetChildAt(i);
        if (Child)
        {
            Child->RemoveFromParent();
        }
    }

    // Добавляем новый виджет
    AddChild(NewWidget);

    // Добавляем обратно все виджеты ниже
    for (int32 i = Index + 1; i < Widgets.Num(); i++)
    {
        AddChild(Widgets[i]);
    }
}