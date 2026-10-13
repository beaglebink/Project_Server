#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"

class IPropertyHandle;
class AActor;

/**
 * Единый customization для FSceneActorRef и FWorldStateActorRef.
 *
 * Раскладка (в порядке отображения):
 *   SceneKind → SceneFloor / SceneRegion → Actor (picker).
 *
 * Набор актёров в пикере определяется meta SceneActorFilter на UPROPERTY,
 * на которой висит структура:
 *   - "InteractiveItem"     → FloorAssignmentComponent + InteractiveItemComponent;
 *   - "FloorAssignmentOnly" → только FloorAssignmentComponent.
 *   При отсутствии meta используется "InteractiveItem".
 */
class FSceneActorRefCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(
        TSharedRef<IPropertyHandle> StructPropertyHandle,
        FDetailWidgetRow& HeaderRow,
        IPropertyTypeCustomizationUtils& Utils) override;

    virtual void CustomizeChildren(
        TSharedRef<IPropertyHandle> StructPropertyHandle,
        IDetailChildrenBuilder& ChildBuilder,
        IPropertyTypeCustomizationUtils& Utils) override;

private:
    enum class EFilterMode : uint8
    {
        InteractiveItem,
        FloorAssignmentOnly
    };

    EFilterMode ReadFilterMode(const TSharedRef<IPropertyHandle>& StructPropertyHandle) const;
    bool ActorMatchesFilter(AActor* Actor, EFilterMode Mode) const;

    // Резолвит UWorld по текущему SceneKind + SceneFloor/SceneRegion.
    UWorld* ResolveSceneWorld() const;

    // Список актёров текущего уровня, подходящих фильтру.
    TArray<AActor*> CollectCandidateActors() const;

    // Текст для превью в header и для кнопки пикера.
    FText GetCurrentActorLabel() const;

    // Меню пикера и обработчики выбора.
    TSharedRef<SWidget> OnGenerateActorMenu();
    FReply OnActorSelected(TWeakObjectPtr<AActor> WeakActor);
    FReply OnClearSelection();

    // ---- Handles ----
    TSharedPtr<IPropertyHandle> StructHandle;
    TSharedPtr<IPropertyHandle> SceneKindHandle;
    TSharedPtr<IPropertyHandle> SceneFloorHandle;
    TSharedPtr<IPropertyHandle> SceneRegionHandle;
    TSharedPtr<IPropertyHandle> PersistentIdHandle;
    TSharedPtr<IPropertyHandle> ActorLabelHandle;

    EFilterMode CurrentFilterMode = EFilterMode::InteractiveItem;
};