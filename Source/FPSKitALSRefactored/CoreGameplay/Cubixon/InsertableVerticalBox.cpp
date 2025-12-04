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

// UInsertableVerticalBox::InsertWidgetAt (Исправленный)
void UInsertableVerticalBox::InsertWidgetAt(UWidget* NewWidget, int32 Index)
{
    if (!NewWidget) return;

    // 1. Убеждаемся, что C++ массив Widgets синхронизирован с UMG-контейнером
    InitializeWidgetsArray();

    // 2. Вставляем новый виджет в C++ массив
    if (!Widgets.Contains(NewWidget))
    {
        // Убеждаемся, что индекс находится в пределах [0, Num()]
        Index = FMath::Clamp(Index, 0, Widgets.Num());
        Widgets.Insert(NewWidget, Index);
    }

    // 3. ПОЛНАЯ ОЧИСТКА UMG-контейнера
    // Вызов ClearChildren() гарантирует, что Slate-слоты удалены.
    // Ваша реализация ClearChildren() также очищает массив Widgets,
    // но поскольку мы его только что заполнили/изменили, мы должны его сохранить.
    // !!! ВАЖНО: Ваша ClearChildren() очищает Widgets. Empty(). Измените ее, 
    // чтобы она не очищала Widgets, если вы хотите использовать этот подход.

    // Временно вызовем Super::ClearChildren() (только для очистки UMG/Slate):
    Super::ClearChildren();

    // 4. Добавляем ВСЕ виджеты из C++ массива обратно в UMG-контейнер
    for (UWidget* ChildWidget : Widgets)
    {
        // AddChild ВСЕГДА добавляет в КОНЕЦ списка, 
        // поэтому порядок в цикле будет правильным.
        AddChild(ChildWidget);
    }

    // 5. Принудительная перекомпоновка (для ScrollBox/Clipping)
    ForceLayoutPrepass();
}

// Изменение в UInsertableVerticalBox::ClearChildren (если нужно сохранять Widgets)
// Если вы хотите, чтобы ClearChildren очищала и UMG, и C++ массив, оставьте как есть.
// Но если вы вызываете ее для перестроения, она должна сохранять массив Widgets.
// Для данного случая, где мы вызываем Super::ClearChildren(), это не так критично.

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

