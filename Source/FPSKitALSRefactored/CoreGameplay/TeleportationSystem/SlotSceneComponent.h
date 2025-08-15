#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TeleportingSubsystem.h"
#include "SlotSceneComponent.generated.h"

class UArrowComponent;
class UTextRenderComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChangeActive, USlotSceneComponent*, Slot, bool, IsActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStopSlotCooldown, ATeleportDestination*, Destination, USlotSceneComponent*, Slot);


UCLASS(ClassGroup = (Custom), BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, meta = (BlueprintSpawnableComponent), HideCategories = ("Rendering", "Physics", "Mobility", "LOD", "Tags", "AssetUserData", "Activation", "Navigation", "Cooking"))
class FPSKITALSREFACTORED_API USlotSceneComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    USlotSceneComponent();

    void SetOwnerName(const FString Name);
    virtual void OnRegister() override;

	UFUNCTION(BlueprintCallable, Category = "Slot")
    void SetActiveSlot(bool bActive);

    UFUNCTION(BlueprintCallable, Category = "Slot")
    bool GetActiveSlot() const;

    UFUNCTION(BlueprintCallable, Category = "Slot")
	FName GetSlotName() const { return SlotName; }

    UFUNCTION(BlueprintCallable, Category = "Slot")
    void SetCoolDownTime(float Time) { CoolDownTime = Time; }

    UFUNCTION(BlueprintCallable, Category = "Slot")
    float GetCoolDownTime() const { return CoolDownTime; }

    UFUNCTION(BlueprintCallable, Category = "Slot")
	bool IsInCooldown() const { return isCooldown; }

    UFUNCTION(BlueprintCallable, Category = "Slot")
    void StartCooldown();

    void OnComponentDestroyed(bool bDestroyingHierarchy);

#if WITH_EDITOR
    void UpdateVisualsFromName();
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void PostDuplicate(bool bDuplicateForPIE) override;
    virtual void PostEditUndo() override;
private:
    void EnsureHelpers();
    void TryRenameObjectToMatchSlotName();
    static FName MakeSafeUniqueName(UObject* Outer, UClass* Class, const FName& Desired);
#endif

public:
    UPROPERTY(EditAnywhere, Category = "Slot")
    FName SlotName = NAME_None;

    UPROPERTY(BlueprintAssignable, Category = "Slot")
	FOnChangeActive OnChangeActive;

	UPROPERTY(EditInstanceOnly, Category = "Slot", meta = (AllowPrivateAccess = "true"))
	bool IsActiveSlot = true;

    UPROPERTY(EditInstanceOnly, Category = "Slot", meta = (AllowPrivateAccess = "true"))
    float CoolDownTime = 0.0f;

    FOnStopSlotCooldown OnStopSlotCooldown;

#if WITH_EDITORONLY_DATA
protected:
    UPROPERTY(Transient)
    TObjectPtr<UArrowComponent> Arrow = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextRenderComponent> Label = nullptr;
#endif

private:
    FString OwnerName;
	bool isCooldown = false;
};
