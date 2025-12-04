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

    InitializeWidgetsArray();

    if (!Widgets.Contains(NewWidget))
    {
        Index = FMath::Clamp(Index, 0, Widgets.Num());
        Widgets.Insert(NewWidget, Index);
    }

    Super::ClearChildren();

    for (UWidget* ChildWidget : Widgets)
    {
        AddChild(ChildWidget);
    }

    ForceLayoutPrepass();
}

void UInsertableVerticalBox::ClearChildren()
{
    Widgets.Empty();

    Super::ClearChildren();
}

void UInsertableVerticalBox::RemoveWidgetFromArray(UWidget* Widget)
{
    if (!Widget)
    {
        return;
    }

    InitializeWidgetsArray();

    int32 RemovedCount = Widgets.RemoveSingle(Widget);
    if (RemovedCount > 0)
    {
        RemoveChild(Widget);
    }
}

