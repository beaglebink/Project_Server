#if WITH_EDITOR

#pragma once

#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "Templates/SharedPointer.h"
#include "UObject/WeakObjectPtr.h"

class IDetailLayoutBuilder;

class FTeleportDestinationDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    FReply OnAddSlotClicked();

    TWeakObjectPtr<class ATeleportDestination> EditedActor;

    IDetailLayoutBuilder* CachedDetailBuilder = nullptr;
};
#endif // WITH_EDITOR
