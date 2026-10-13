#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "SceneActorRef.h"
#include "InteractSetEnabledPayload.generated.h"

/**
 * Payload для команды включения/выключения интерактивного компонента.
 *
 * Два способа задания цели:
 *   1. TargetActor — scene + persistent actor ID. Дизайнер задаёт из редактора
 *      через dropdown. Используется сценариями (PayloadBatchEntry).
 *   2. ItemId — рантайм-GUID. Заполняется автоматически
 *      FInteractiveSubsystemMethods::PublishInteractSetEnabled.
 *
 * WorldStateSubsystem резолвит:
 *   - если TargetActor задан → персистентный ID берётся из него;
 *   - иначе по ItemId ищется актёр на сцене и берётся его
 *     FloorAssignmentComponent::ItemId.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteractSetEnabledPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    // --- Designer-facing ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractSetEnabled",
        meta = (SceneActorFilter = "InteractiveItem"))
    FSceneActorRef TargetActor;

    // --- Runtime-facing ---
    UPROPERTY(BlueprintReadWrite, Category = "InteractSetEnabled")
    FGuid ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteractSetEnabled")
    bool bEnabled = true;

    // Runtime-конструктор (для FInteractiveSubsystemMethods).
    UFUNCTION(BlueprintCallable, Category = "InteractSetEnabled")
    UInteractSetEnabledPayload* Setup(const FGuid& InItemId, bool bInEnabled)
    {
        ItemId = InItemId;
        bEnabled = bInEnabled;
        return this;
    }

    // Editor-конструктор (для сценарных BP).
    UFUNCTION(BlueprintCallable, Category = "InteractSetEnabled")
    UInteractSetEnabledPayload* SetupPersistent(const FSceneActorRef& InTarget, bool bInEnabled)
    {
        TargetActor = InTarget;
        bEnabled = bInEnabled;
        return this;
    }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractSetEnabled")
    FGuid GetItemId() const { return ItemId; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractSetEnabled")
    bool IsEnabled() const { return bEnabled; }
};