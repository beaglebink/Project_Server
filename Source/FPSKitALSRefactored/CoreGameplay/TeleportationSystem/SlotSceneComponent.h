#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SlotSceneComponent.generated.h"

class UArrowComponent;
class UTextRenderComponent;

UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, meta = (BlueprintSpawnableComponent), HideCategories = ("Rendering", "Physics", "Mobility", "LOD", "Tags", "AssetUserData", "Activation", "Navigation", "Cooking"))
class FPSKITALSREFACTORED_API USlotSceneComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    USlotSceneComponent();

    // Единственный источник правды
    UPROPERTY(EditAnywhere, Category = "Slot")
    FName SlotName = NAME_None;

    void SetOwnerName(const FString Name);

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
    FString OwnerName;

#if WITH_EDITOR
public:
    void UpdateVisualsFromName();
private:
    void EnsureHelpers();
    void TryRenameObjectToMatchSlotName();
    static FName MakeSafeUniqueName(UObject* Outer, UClass* Class, const FName& Desired);
#endif
};
