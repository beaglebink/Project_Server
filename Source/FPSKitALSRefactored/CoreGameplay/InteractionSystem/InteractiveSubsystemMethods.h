#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

#include "InteractiveItemComponent.h"
#include "InteractCommandPayload.h"
#include "InteractSetEnabledPayload.h"
#include "EventBusSubsystem.h"
#include "OutcomeEventBase.h"

class UInteractivePickerComponent;

/**
 * Миксин с реализацией общих методов для per-item listener API и helper'ов
 * исполнения интеракции.
 *
 * Изменения:
 *   - ExecuteSetEnabledOnOwner теперь публикует UInteractSetEnabledPayload
 *     через EventBus. Это позволяет UWorldStateSubsystem перехватить
 *     изменение и сохранить его как персистентный факт (переживает смену
 *     уровня и save/load).
 *   - Каждая вспомогательная функция определения "какая категория события"
 *     выделена в отдельный static helper (MakeEnabledEventCategory).
 */
class FInteractiveSubsystemMethods
{
public:
    virtual ~FInteractiveSubsystemMethods() = default;

    // =========================================================================
    // Per-item listener API
    // =========================================================================

    // Добавляет слушателя для конкретного ItemId (устраняет дубликаты, чистит невалидные)
    void AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
    {
        if (!Listener) return;

        TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& Map = GetRegistrationListeners();
        TArray<TWeakObjectPtr<UInteractiveItemComponent>>& Arr = Map.FindOrAdd(ItemId);

        for (int32 i = Arr.Num() - 1; i >= 0; --i)
        {
            if (!Arr[i].IsValid()) Arr.RemoveAtSwap(i);
        }

        for (const TWeakObjectPtr<UInteractiveItemComponent>& W : Arr)
        {
            if (W.IsValid() && W.Get() == Listener) return;
        }

        Arr.Add(Listener);
    }

    // Удаляет слушателя для конкретного ItemId, чистит невалидные записи
    void RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
    {
        if (!Listener) return;

        TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& Map = GetRegistrationListeners();
        if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* Arr = Map.Find(ItemId))
        {
            for (int32 i = Arr->Num() - 1; i >= 0; --i)
            {
                if (!(*Arr)[i].IsValid() || (*Arr)[i].Get() == Listener)
                {
                    Arr->RemoveAtSwap(i);
                }
            }
            if (Arr->Num() == 0)
            {
                Map.Remove(ItemId);
            }
        }
    }

    // =========================================================================
    // Interaction execution helpers
    // =========================================================================

    // ----- Нажатие: выполнить интеракцию -----
    bool ExecuteInteractCommandOnOwner(const FGuid& ItemId, AActor* Owner, UInteractivePickerComponent* Picker)
    {
        if (!Owner) return false;

        TInlineComponentArray<UInteractiveItemComponent*> Components;
        Owner->GetComponents<UInteractiveItemComponent>(Components);

        for (UInteractiveItemComponent* Comp : Components)
        {
            if (Comp && Comp->IsActive() && Comp->GetItemId() == ItemId)
            {
                Comp->OnInteractionPressKeyEvent.Broadcast(Picker);
                return true;
            }
        }
        return false;
    }

    // ----- Включение/выключение интеракции -----
    // Изменяет активность компонента И публикует UInteractSetEnabledPayload
    // через EventBus, чтобы подсистема WorldState могла сохранить изменение.
    bool ExecuteSetEnabledOnOwner(const FGuid& ItemId, AActor* Owner, bool bEnabled)
    {
        if (!Owner) return false;

        TInlineComponentArray<UInteractiveItemComponent*> Components;
        Owner->GetComponents<UInteractiveItemComponent>(Components);

        for (UInteractiveItemComponent* Comp : Components)
        {
            if (!Comp || Comp->GetItemId() != ItemId)
                continue;

            // 1) Меняем состояние компонента (SetActive + bInteractionEnabled).
            Comp->SetIsActive(bEnabled);

            // 2) Публикуем событие, чтобы подсистема WorldState зарегистрировала
            //    факт и сохранила его в сейве / переприменила при смене уровня.
            PublishInteractSetEnabled(Owner, ItemId, bEnabled);

            UE_LOG(LogTemp, Log,
                TEXT("FInteractiveSubsystemMethods: SetEnabled ItemId=%s bEnabled=%s Owner=%s (Установлен флаг активности)"),
                *ItemId.ToString(),
                bEnabled ? TEXT("true") : TEXT("false"),
                *Owner->GetName());
            return true;
        }
        return false;
    }

    // ----- Изменение радиуса интеракции -----
    bool ExecuteSetRangeOnOwner(const FGuid& ItemId, AActor* Owner, float NewRange)
    {
        if (!Owner) return false;
        TInlineComponentArray<UInteractiveItemComponent*> Components;
        Owner->GetComponents<UInteractiveItemComponent>(Components);

        for (UInteractiveItemComponent* Comp : Components)
        {
            if (Comp && Comp->GetItemId() == ItemId)
            {
                Comp->InteractionRange = NewRange;
                UE_LOG(LogTemp, Log,
                    TEXT("FInteractiveSubsystemMethods: SetRange ItemId=%s NewRange=%.2f Owner=%s (Диапазон обновлён)"),
                    *ItemId.ToString(),
                    NewRange,
                    *Owner->GetName());
                // Optionally notify component about change (component may broadcast itself when appropriate)
                return true;
            }
        }
        return false;
    }

    // ----- Изменение подсказки интеракции -----
    bool ExecuteSetTooltipOnOwner(const FGuid& ItemId, AActor* Owner, const FText& NewTooltip)
    {
        if (!Owner) return false;
        TInlineComponentArray<UInteractiveItemComponent*> Components;
        Owner->GetComponents<UInteractiveItemComponent>(Components);

        for (UInteractiveItemComponent* Comp : Components)
        {
            if (Comp && Comp->GetItemId() == ItemId)
            {
                Comp->SetTooltip(NewTooltip);
                UE_LOG(LogTemp, Log,
                    TEXT("FInteractiveSubsystemMethods: SetTooltip ItemId=%s Owner=%s Tooltip=%s (Тултип обновлён)"),
                    *ItemId.ToString(), *Owner->GetName(), *NewTooltip.ToString());
                return true;
            }
        }
        return false;
    }

protected:
    virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() = 0;

private:
    // =========================================================================
    // EventBus publishing
    // =========================================================================

    // Определяет категорию события по типу подсистемы, которой принадлежит
    // компонент. Это соответствует тому, какой EOutcomeType используется
    // во всём остальном коде (InteractRegistered / InteractUnregistered).
    static void ApplyCategoryToEvent(const UInteractiveItemComponent* Comp, FOutcomeEventBase& OutEvent)
    {
        if (!Comp)
        {
            OutEvent.OutcomeType = EOutcomeType::Interior;
            OutEvent.OutcomeInterior = EOutcomeInterior::InteractSetEnabled;
            return;
        }

        switch (Comp->SubsystemType)
        {
        case EInteractiveSubsystem::Terminal:
            OutEvent.OutcomeType = EOutcomeType::Terminal;
            OutEvent.OutcomeTerminal = EOutcomeTerminal::InteractSetEnabled;
            break;
        case EInteractiveSubsystem::ActorNPC:
            OutEvent.OutcomeType = EOutcomeType::Actor;
            OutEvent.OutcomeActor = EOutcomeActor::InteractSetEnabled;
            break;
        case EInteractiveSubsystem::Inventory:
            OutEvent.OutcomeType = EOutcomeType::Inventory;
            OutEvent.OutcomeInventory = EOutcomeInventory::InteractSetEnabled;
            break;
        case EInteractiveSubsystem::Interior:
        default:
            OutEvent.OutcomeType = EOutcomeType::Interior;
            OutEvent.OutcomeInterior = EOutcomeInterior::InteractSetEnabled;
            break;
        }
    }

    // Находит компонент-владелец по ItemId и публикует UInteractSetEnabledPayload.
    void PublishInteractSetEnabled(AActor* Owner, const FGuid& ItemId, bool bEnabled)
    {
        if (!Owner) return;

        UWorld* World = Owner->GetWorld();
        if (!World) return;

        UGameInstance* GI = World->GetGameInstance();
        if (!GI) return;

        UEventBusSubsystem* Bus = GI->GetSubsystem<UEventBusSubsystem>();
        if (!Bus) return;

        // Находим компонент, чтобы понять его SubsystemType (→ категорию события).
        UInteractiveItemComponent* FoundComp = nullptr;
        TInlineComponentArray<UInteractiveItemComponent*> Components;
        Owner->GetComponents<UInteractiveItemComponent>(Components);
        for (UInteractiveItemComponent* Comp : Components)
        {
            if (Comp && Comp->GetItemId() == ItemId)
            {
                FoundComp = Comp;
                break;
            }
        }

        UInteractSetEnabledPayload* P = Bus->CreatePayload<UInteractSetEnabledPayload>();
        if (!P) return;

        P->Setup(ItemId, bEnabled);

        FOutcomeEventBase Event;
        ApplyCategoryToEvent(FoundComp, Event);
        Event.Payload = P;

        Bus->PublishOutcome(Event);
    }
};