#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SlotSceneComponent.h"
#include "Components/BillboardComponent.h"
#include "TeleportDestination.generated.h"

//class USlotSceneComponent;

UCLASS()
class FPSKITALSREFACTORED_API ATeleportDestination : public AActor
{
    GENERATED_BODY()

public:
    ATeleportDestination();

    void OnConstruction(const FTransform& Transform);

    void BeginPlay();

    void EndPlay(const EEndPlayReason::Type EndPlayReason);

    // Root-���������, � �������� ����� �������������
    UPROPERTY(VisibleDefaultsOnly, Category = "Slots")
    TObjectPtr<USceneComponent> Root = nullptr;

    // ������������� ����� ����������, ������������ ������� �����������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
    FString DestinationID;

    // ��� ����� ������������, ��� Instanced-����������
    UPROPERTY(EditAnywhere, Category = "Teleportation", Instanced, meta = (TitleProperty = "SlotName"))
    TArray<TObjectPtr<USlotSceneComponent>> Slots;

    // ������� ������ � Details Panel � ����� �� ����������
    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Slots")
    USlotSceneComponent* AddSlot();
    /*
    UPROPERTY(EditAnywhere, Category = "Slot Config", meta = (AllowAbstract = false, MustImplement = "SlotInterface"))
    TSubclassOf<USlotSceneComponent> DesiredSlotType = USlotSceneComponent::StaticClass();
    */
    UPROPERTY(Transient)
    UBillboardComponent* LabelComponent;

    UPROPERTY(Transient)
    TObjectPtr<UTextRenderComponent> Label = nullptr;

#if WITH_EDITOR
    // ������������ ��������� ������� Slots (������ ����������)
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

    // ��������� Undo/Redo � �������������� �������� � �����������
    virtual void PostEditUndo() override;

private:
    // ���������� ��� ��� SlotName � ����� �������
    FName GenerateUniqueSlotName(const FName& Base = TEXT("Slot")) const;

    // �������� �� ������ ���������� �� Root, ����������� �����������
    void EnsureSlotAttached(USlotSceneComponent* Slot);
#endif
};