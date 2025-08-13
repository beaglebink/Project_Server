#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SlotSceneComponent.h"
#include "Components/BillboardComponent.h"
#include "TeleportDestination.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChangeActiveDestination, ATeleportDestination*, Destination, bool, IsActive);

UCLASS()
class FPSKITALSREFACTORED_API ATeleportDestination : public AActor
{
    GENERATED_BODY()

public:
    ATeleportDestination();

    void OnConstruction(const FTransform& Transform);

    void BeginPlay();

    void EndPlay(const EEndPlayReason::Type EndPlayReason);

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Slots")
    USlotSceneComponent* AddSlot();

    UFUNCTION(BlueprintCallable, Category = "Slot")
    TArray<USlotSceneComponent*> GetSlots() const;

	UFUNCTION(BlueprintCallable, Category = "Slot")
    USlotSceneComponent* GetSlotByName(const FName& SlotName) const;

    UFUNCTION(BlueprintCallable, Category = "Slot")
    USlotSceneComponent* GetSlotByIndex(int32 Index) const;

    UFUNCTION(BlueprintCallable, Category = "Teleportation")
    void SetActiveDestination(bool bActive);

    UFUNCTION(BlueprintCallable, Category = "Teleportation")
    bool GetActiveDestination() const;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

    virtual void PostEditUndo() override;

private:
    void EnsureSlotAttached(USlotSceneComponent* Slot);
    FName GenerateUniqueSlotName(const FName& Base = TEXT("Slot")) const;
#endif

public:
    UPROPERTY(VisibleDefaultsOnly, Category = "Slots")
    TObjectPtr<USceneComponent> Root = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
    FString DestinationID;

    UPROPERTY(EditAnywhere, Category = "Teleportation", Instanced, meta = (TitleProperty = "SlotName"))
    TArray<TObjectPtr<USlotSceneComponent>> Slots;

    UPROPERTY(Transient)
    UBillboardComponent* LabelComponent;

    UPROPERTY(Transient)
    TObjectPtr<UTextRenderComponent> Label = nullptr;

    UPROPERTY(EditInstanceOnly, Category = "Teleportation", meta = (AllowPrivateAccess = "true"))
    bool IsActiveDestination = true;

    UPROPERTY(BlueprintAssignable, Category = "Teleportation")
	FOnChangeActiveDestination OnChangeActiveDestination;
};