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

	// Добавляет слушателя для конкретного ItemId (устраняются дубликаты).
	// Улучшение: перед добавлением очищаем невалидные weak-референсы, чтобы не накапливать "мёртвые" записи.
	void AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
	{
		if (!Listener) return;

		TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& Map = GetRegistrationListeners();
		TArray<TWeakObjectPtr<UInteractiveItemComponent>>& Arr = Map.FindOrAdd(ItemId);

		// Очистка невалидных weak-указателей в массиве
		for (int32 i = Arr.Num() - 1; i >= 0; --i)
		{
			if (!Arr[i].IsValid())
			{
				Arr.RemoveAtSwap(i);
			}
		}

		// Проверка на дубликат
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
				// Удаляем либо невалидные weak, либо совпадающий слушатель
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

	// ----- Интеракция -----
	// Выполнить интеракцию на уже найденном актёре-владельце (Owner) — ищет активный компонент с ItemId и вызывает Broadcast
	// Возвращает true если интеракция была выполнена
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

	// Включить/выключить интерактивный компонент — ищет компонент по ItemId и вызывает SetIsActive
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
				UE_LOG(LogTemp, Log, TEXT("FInteractiveSubsystemMethods: SetEnabled ItemId=%s bEnabled=%s Owner=%s"),
					*ItemId.ToString(),
					bEnabled ? TEXT("true") : TEXT("false"),
					*Owner->GetName());
				return true;
			}
		}
		return false;
	}

protected:
	virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() = 0;
};