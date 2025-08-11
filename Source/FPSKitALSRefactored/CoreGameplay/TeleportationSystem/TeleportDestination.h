#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TeleportDestination.generated.h"

class USlotSceneComponent;

UCLASS()
class FPSKITALSREFACTORED_API ATeleportDestination : public AActor
{
    GENERATED_BODY()

public:
    ATeleportDestination();

    // Root-���������, � �������� ����� �������������
    UPROPERTY(VisibleDefaultsOnly, Category = "Slots")
    TObjectPtr<USceneComponent> Root = nullptr;

    // ��� ����� ������������, ��� Instanced-����������
    UPROPERTY(EditAnywhere, Category = "Slots", Instanced, meta = (TitleProperty = "SlotName"))
    TArray<TObjectPtr<USlotSceneComponent>> Slots;

    // ������������� ����� ����������, ������������ ������� �����������
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
    FString DestinationID;

    // ������� ������ � Details Panel � ����� �� ����������
    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Slots")
    USlotSceneComponent* AddSlot();

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