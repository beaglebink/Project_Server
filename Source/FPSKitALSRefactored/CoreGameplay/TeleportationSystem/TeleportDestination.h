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

    // Root-компонент, к которому слоты прикрепляются
    UPROPERTY(VisibleDefaultsOnly, Category = "Slots")
    TObjectPtr<USceneComponent> Root = nullptr;

    // Идентификатор точки назначения, используется логикой перемещения
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleportation")
    FString DestinationID;

    // Все слоты телепортации, как Instanced-компоненты
    UPROPERTY(EditAnywhere, Category = "Teleportation", Instanced, meta = (TitleProperty = "SlotName"))
    TArray<TObjectPtr<USlotSceneComponent>> Slots;

    // Удобная кнопка в Details Panel и вызов из блюпринтов
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
    // Отслеживание изменений массива Slots (ручное добавление)
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

    // Поддержка Undo/Redo — восстановление привязки и регистрации
    virtual void PostEditUndo() override;

private:
    // Уникальное имя для SlotName и имени объекта
    FName GenerateUniqueSlotName(const FName& Base = TEXT("Slot")) const;

    // Защищает от отрыва компонента от Root, гарантирует регистрацию
    void EnsureSlotAttached(USlotSceneComponent* Slot);
#endif
};