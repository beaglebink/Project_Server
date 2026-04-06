#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractiveActorInterface.generated.h"

UINTERFACE(Blueprintable)
class FPSKITALSREFACTORED_API UInteractiveActorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Интерфейс для актёров, содержащих интерактивный компонент.
 * Реализуется в Blueprint/C++.
 * Методы:
 *  - EnableHighlight: включить/выключить визуальную подсветку (например, материал/декаль/постпроцесс).
 *  - SetInteractiveEnabled: включить/выключить возможность интеракции (логика подсистемы использует это состояние).
 *  - CanInteract: вернуть, можно ли сейчас взаимодействовать.
 *  - GetInteractionTooltip: текст тултипа для UI.
 */
class FPSKITALSREFACTORED_API IInteractiveActorInterface
{
	GENERATED_BODY()

public:
	// Включить/выключить визуальную подсветку
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interactive")
	void EnableHighlight(bool bEnable);
	virtual void EnableHighlight_Implementation(bool bEnable) {}
};