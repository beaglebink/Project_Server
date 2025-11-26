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

    for (int32 i = Index; i < GetChildrenCount(); i++)
    {
        UWidget* Child = GetChildAt(i);
        if (Child)
        {
            Child->RemoveFromParent();
        }
    }

    AddChild(NewWidget);

    for (int32 i = Index + 1; i < Widgets.Num(); i++)
    {
        AddChild(Widgets[i]);
    }
}