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
        // �������� ��������� �����
        ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
        if (LocalPlayer)
        {
            // �������� ���������� Enhanced Input
            UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
            if (InputSubsystem)
            {
                // ������� �������� �����
                FInputActionValue InputValue(true); // ��� boolean ��������

                // ����������� �������� ����� ����������
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
