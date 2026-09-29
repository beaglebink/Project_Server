#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TerminalSubsystem.h"
#include "TerminalProfileProvider.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UTerminalProfileProvider : public UInterface
{
    GENERATED_BODY()
};

class FPSKITALSREFACTORED_API ITerminalProfileProvider
{
    GENERATED_BODY()

public:
    /** Return the display name of this profile. */
    /** Возвращает отображаемое имя этого профиля. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TerminalProfile")
    FString GetProfileName() const;

    /** Return the terminal capabilities. */
    /** Возвращает возможности терминала. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TerminalProfile")
    TArray<ETerminalCapability> GetTerminalCapabilities() const;

    /** Whether the terminal should be locked by default. */
    /** Должен ли терминал быть заблокирован по умолчанию. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TerminalProfile")
    bool IsTerminalLockedByDefault() const;

    /** Return the password (if any) for the terminal. */
    /** Возвращает пароль (если есть) для терминала. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TerminalProfile")
    FString GetTerminalPassword() const;

    /** Return the initial files for the terminal (flat list with optional ParentPath for hierarchy). */
    /** Возвращает начальные файлы для терминала (плоский список с опциональным ParentPath для иерархии). */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TerminalProfile")
    TArray<FTerminalFileEntry> GetDefaultFiles() const;

    /** Optional: apply additional default global data (email, websites, etc.) – can be implemented if needed. */
    /** Опционально: применить дополнительные глобальные данные по умолчанию (почта, сайты и т.д.) – можно реализовать при необходимости. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TerminalProfile")
    void ApplyGlobalDefaultData(UPARAM(ref) FTerminalGlobalState& GlobalState) const;
};