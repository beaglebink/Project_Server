#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TerminalSubsystem.h" // for ETerminalCapability, FTerminalFileEntry
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
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TerminalProfile")
    FString GetProfileName() const;

    /** Return the terminal capabilities. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TerminalProfile")
    TArray<ETerminalCapability> GetTerminalCapabilities() const;

    /** Whether the terminal should be locked by default. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TerminalProfile")
    bool IsTerminalLockedByDefault() const;

    /** Return the password (if any) for the terminal. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TerminalProfile")
    FString GetTerminalPassword() const;

    /** Return the initial files for the terminal (flat list with optional ParentPath for hierarchy). */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TerminalProfile")
    TArray<FTerminalFileEntry> GetDefaultFiles() const;

    /** Optional: apply additional default global data (email, websites, etc.) – can be implemented if needed. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "TerminalProfile")
    void ApplyGlobalDefaultData(UPARAM(ref) FTerminalGlobalState& GlobalState) const;
};