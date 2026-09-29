#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "TerminalSubsystem.h"
#include "TerminalCommandPayload.generated.h" 

// ============================================================================
// Existing types
// Существующие типы
// ============================================================================

UENUM(BlueprintType)
enum class ECommandObjectType : uint8
{
    ModeratelyReactive  UMETA(DisplayName = "Moderately Reactive"),
    Door                UMETA(DisplayName = "Door"),
    Terminal            UMETA(DisplayName = "Terminal")
};

// Existing payload of the "test" interaction command.
// Существующий payload «тестовой» команды взаимодействия.
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UTerminalCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Terminal Command")
    FGuid ObjectItemId;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Terminal Command")
    ECommandObjectType ObjectType;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Terminal Command")
    bool IsEnable;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Terminal Command")
    AActor* OwnerActor;
};

// ============================================================================
// NEW: command payloads for UTerminalSubsystem.
//
// They are sent through UEventBusSubsystem::PublishOutcome with the fields:
//   OutcomeType     = EOutcomeType::Terminal
//   OutcomeTerminal = EOutcomeTerminal::<X>Request
//
// The handlers in UTerminalSubsystem call the corresponding private
// mutators. The public API of the subsystem is only getters.
//
// НОВЫЕ: командные payload'ы для UTerminalSubsystem.
//
// Отправляются через UEventBusSubsystem::PublishOutcome с полями:
//   OutcomeType     = EOutcomeType::Terminal
//   OutcomeTerminal = EOutcomeTerminal::<X>Request
//
// Обработчики в UTerminalSubsystem вызывают соответствующие приватные
// мутаторы. Публичный API подсистемы — только геттеры.
// ============================================================================

// ---- Common payload with a single TerminalId (for commands without extra data) --------
// ---- Общий payload с одним TerminalId (для команд без доп. данных) --------
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalIdCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FGuid TerminalId;

    UFUNCTION(BlueprintCallable, Category = "Terminal")
    UTerminalIdCommandPayload* Setup(const FGuid& InId)
    {
        TerminalId = InId;
        return this;
    }
};

// ---- Register / Unregister ------------------------------------------------
// ---- Регистрация / Отмена регистрации ------------------------------------------------
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalRegisterCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FGuid TerminalId;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    TObjectPtr<AActor> ProfileActor;

    UFUNCTION(BlueprintCallable, Category = "Terminal")
    UTerminalRegisterCommandPayload* Setup(const FGuid& InId, AActor* InProfile)
    {
        TerminalId = InId;
        ProfileActor = InProfile;
        return this;
    }
};

// Unregister uses UTerminalIdCommandPayload (only TerminalId).
// Unregister использует UTerminalIdCommandPayload (только TerminalId).

// ---- ApplyProfile ---------------------------------------------------------
// ---- Применить профиль ---------------------------------------------------------
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalApplyProfileCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FGuid TerminalId;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    TObjectPtr<AActor> ProfileActor;

    UFUNCTION(BlueprintCallable, Category = "Terminal")
    UTerminalApplyProfileCommandPayload* Setup(const FGuid& InId, AActor* InProfile)
    {
        TerminalId = InId;
        ProfileActor = InProfile;
        return this;
    }
};

// ---- SetCapabilities ------------------------------------------------------
// ---- Установить возможности ------------------------------------------------------
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalSetCapabilitiesCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FGuid TerminalId;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    TArray<ETerminalCapability> Capabilities;

    UFUNCTION(BlueprintCallable, Category = "Terminal")
    UTerminalSetCapabilitiesCommandPayload* Setup(const FGuid& InId,
        const TArray<ETerminalCapability>& InCaps)
    {
        TerminalId = InId;
        Capabilities = InCaps;
        return this;
    }
};

// ---- Login (password / account) -------------------------------------------
// ---- Вход (пароль / аккаунт) -------------------------------------------
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalLoginPasswordCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FGuid TerminalId;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FString Password;

    UFUNCTION(BlueprintCallable, Category = "Terminal")
    UTerminalLoginPasswordCommandPayload* Setup(const FGuid& InId, const FString& InPassword)
    {
        TerminalId = InId;
        Password = InPassword;
        return this;
    }
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalLoginAccountCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FGuid TerminalId;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FString AccountName;

    UFUNCTION(BlueprintCallable, Category = "Terminal")
    UTerminalLoginAccountCommandPayload* Setup(const FGuid& InId, const FString& InAccount)
    {
        TerminalId = InId;
        AccountName = InAccount;
        return this;
    }
};

// ---- ContextualAccess -----------------------------------------------------
// ---- Контекстный доступ -----------------------------------------------------
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalContextualAccessCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FGuid TerminalId;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FString ContextToken;

    UFUNCTION(BlueprintCallable, Category = "Terminal")
    UTerminalContextualAccessCommandPayload* Setup(const FGuid& InId, const FString& InToken)
    {
        TerminalId = InId;
        ContextToken = InToken;
        return this;
    }
};

// ---- Write / Delete file --------------------------------------------------
// ---- Запись / Удаление файла --------------------------------------------------
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalWriteFileCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FGuid TerminalId;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FString FileName;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FString Content;

    UFUNCTION(BlueprintCallable, Category = "Terminal")
    UTerminalWriteFileCommandPayload* Setup(const FGuid& InId,
        const FString& InFile,
        const FString& InContent)
    {
        TerminalId = InId;
        FileName = InFile;
        Content = InContent;
        return this;
    }
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalDeleteFileCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FGuid TerminalId;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FString FileName;

    UFUNCTION(BlueprintCallable, Category = "Terminal")
    UTerminalDeleteFileCommandPayload* Setup(const FGuid& InId, const FString& InFile)
    {
        TerminalId = InId;
        FileName = InFile;
        return this;
    }
};

// ---- Global services: email / website / shared data ----------------------
// ---- Глобальные сервисы: email / website / общие данные ----------------------
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalSetEmailCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FString Account;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FString Content;

    UFUNCTION(BlueprintCallable, Category = "Terminal")
    UTerminalSetEmailCommandPayload* Setup(const FString& InAccount, const FString& InContent)
    {
        Account = InAccount;
        Content = InContent;
        return this;
    }
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalSetWebsiteCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FString Url;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FString Content;

    UFUNCTION(BlueprintCallable, Category = "Terminal")
    UTerminalSetWebsiteCommandPayload* Setup(const FString& InUrl, const FString& InContent)
    {
        Url = InUrl;
        Content = InContent;
        return this;
    }
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalSetGlobalDataCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FString Key;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FString Value;

    UFUNCTION(BlueprintCallable, Category = "Terminal")
    UTerminalSetGlobalDataCommandPayload* Setup(const FString& InKey, const FString& InValue)
    {
        Key = InKey;
        Value = InValue;
        return this;
    }
};

// ---- Add log --------------------------------------------------------------
// ---- Добавить лог --------------------------------------------------------------
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalAddLogCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FGuid TerminalId;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal")
    FString Message;

    UFUNCTION(BlueprintCallable, Category = "Terminal")
    UTerminalAddLogCommandPayload* Setup(const FGuid& InId, const FString& InMsg)
    {
        TerminalId = InId;
        Message = InMsg;
        return this;
    }
};