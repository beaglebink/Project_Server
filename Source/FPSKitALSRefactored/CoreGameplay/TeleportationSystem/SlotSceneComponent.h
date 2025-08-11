#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SlotSceneComponent.generated.h"

class UArrowComponent;
class UTextRenderComponent;

UCLASS(ClassGroup = (Gameplay), BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class FPSKITALSREFACTORED_API USlotSceneComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    USlotSceneComponent();

    // Единственный источник правды
    UPROPERTY(EditAnywhere, Category = "Slot")
    FName SlotName = NAME_None;

#if WITH_EDITORONLY_DATA
protected:
    UPROPERTY(Transient)
    TObjectPtr<UArrowComponent> Arrow = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextRenderComponent> Label = nullptr;
#endif

    // USceneComponent
    virtual void OnRegister() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void PostDuplicate(bool bDuplicateForPIE) override;
    virtual void PostEditUndo() override;
#endif

private:
#if WITH_EDITOR
    void EnsureHelpers();
    void UpdateVisualsFromName();
    void TryRenameObjectToMatchSlotName();
    static FName MakeSafeUniqueName(UObject* Outer, UClass* Class, const FName& Desired);
#endif
};
