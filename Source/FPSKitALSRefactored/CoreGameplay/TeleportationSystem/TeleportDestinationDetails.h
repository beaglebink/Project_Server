#pragma once

#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtr.h"

class IDetailLayoutBuilder;

/**
 * ������������ Details Panel ��� ATeleportDestination
 */
class FTeleportDestinationDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    // ���������� ������ "Add Slot"
    FReply OnAddSlotClicked();

    // ������ �� ������������� �����
    TWeakObjectPtr<class ATeleportDestination> EditedActor;

    // ������ ��� ������� � ������������ ��������
    IDetailLayoutBuilder* CachedDetailBuilder = nullptr;
};