#pragma once

#include "CoreMinimal.h"
#include "InteractiveItemComponent.h" // <--- нужно, чтобы UInteractiveItemComponent был известен как UObject-подкласс

class UInteractiveItemComponent;

/**
 * Миксин с реализацией общих методов для per-item listener API.
 * Подсистемы должны реализовать GetRegistrationListeners() и хранить свой локальный
 * TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners.
 */
class FInteractiveSubsystemMethods
{
public:
	virtual ~FInteractiveSubsystemMethods() = default;

	// Добавляет слушателя для конкретного ItemId (устраняются дубликаты)
	void AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
	{
		if (!Listener)
		{
			return;
		}

		TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& Map = GetRegistrationListeners();
		TArray<TWeakObjectPtr<UInteractiveItemComponent>>& Arr = Map.FindOrAdd(ItemId);

		for (const TWeakObjectPtr<UInteractiveItemComponent>& W : Arr)
		{
			if (W.Get() == Listener)
			{
				return;
			}
		}
		Arr.Add(Listener);
	}

	// Удаляет слушателя для конкретного ItemId, чистит невалидные записи
	void RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
	{
		if (!Listener)
		{
			return;
		}

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

protected:
	// Подсистема должна предоставить ссылку на свой локальный RegistrationListeners
	virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() = 0;
};