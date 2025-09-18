#include "CoreBlueprintFunctionLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputTriggers.h"
#include "Engine/LocalPlayer.h"

void UCoreBlueprintFunctionLibrary::SimulateKeyPress(APlayerController* PlayerController, UInputAction* InputAction)
{
    if (PlayerController && InputAction)
    {
        // Получаем локальный игрок
        ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
        if (LocalPlayer)
        {
            // Получаем подсистему Enhanced Input
            UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
            if (InputSubsystem)
            {
                // Создаем значение ввода
                FInputActionValue InputValue(true); // Для boolean действия

                // Инжектируем действие через подсистему
                InputSubsystem->InjectInputForAction(InputAction, InputValue, {}, {});
            }
        }
    }
}

bool UCoreBlueprintFunctionLibrary::IsPIE()
{
#if WITH_EDITOR
	return true;
#else
	return false;
#endif
}
