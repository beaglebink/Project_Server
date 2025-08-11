#pragma once

#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtr.h"

class IDetailLayoutBuilder;

/**
 * Кастомизация Details Panel для ATeleportDestination
 */
class FTeleportDestinationDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    // Обработчик кнопки "Add Slot"
    FReply OnAddSlotClicked();

    // Ссылка на редактируемый актор
    TWeakObjectPtr<class ATeleportDestination> EditedActor;

    // Хелпер для доступа к редакторским функциям
    IDetailLayoutBuilder* CachedDetailBuilder = nullptr;
};