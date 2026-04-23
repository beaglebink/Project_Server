#pragma once

#include "CoreMinimal.h"
#include "InteractiveItemComponent.h"
#include "InteractCommandPayload.h"

class UInteractivePickerComponent;

/**
 * Миксин с реализацией общих методов для per-item listener API и helper'ов исполнения интеракции.
 */
class FInteractiveSubsystemMethods
{
public:
	virtual ~FInteractiveSubsystemMethods() = default;

	// Добавляет слушателя для конкретного ItemId (устраняются дубликаты, чистятся невалидные)
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

	// ----- Интеракция: выполнить нажатие -----
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
	bool ExecuteSetEnabledOnOwner(const FGuid& ItemId, AActor* Owner, bool bEnabled)
	{
		if (!Owner) return false;
		TInlineComponentArray<UInteractiveItemComponent*> Components;
		Owner->GetComponents<UInteractiveItemComponent>(Components);

		for (UInteractiveItemComponent* Comp : Components)
		{
			if (Comp && Comp->GetItemId() == ItemId)
			{
                Comp->SetIsActive(bEnabled);
                UE_LOG(LogTemp, Log, TEXT("FInteractiveSubsystemMethods: SetEnabled ItemId=%s bEnabled=%s Owner=%s (Установлен флаг активности)"),
                    *ItemId.ToString(),
                    bEnabled ? TEXT("true") : TEXT("false"),
                    *Owner->GetName());
				return true;
			}
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
                UE_LOG(LogTemp, Log, TEXT("FInteractiveSubsystemMethods: SetRange ItemId=%s NewRange=%.2f Owner=%s (Диапазон обновлён)"),
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
                UE_LOG(LogTemp, Log, TEXT("FInteractiveSubsystemMethods: SetTooltip ItemId=%s Owner=%s Tooltip=%s (Тултип обновлён)"),
                    *ItemId.ToString(), *Owner->GetName(), *NewTooltip.ToString());
				return true;
			}
		}
		return false;
	}

protected:
	virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() = 0;
};