// CheckCondition.cpp
#include "CheckCondition.h"
#include "CheckCoordinatorComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "HAL/IConsoleManager.h"

void UCheckCondition::Initialize(UCheckCoordinatorComponent* InCoordinator, UEventBusSubsystem* InEventBus)
{
    Coordinator = InCoordinator;
    EventBus = InEventBus;

    if (EventBus)
    {
        UOutcomeConditionAsset* Asset = CreateSubscriptionCondition();
        if (Asset)
        {
            EventBus->RegisterHandler(Asset, FOutcomeHandlerDelegate::CreateUObject(this, &UCheckCondition::OnCheckResponse));
        }
    }
}

UWorld* UCheckCondition::GetWorld() const
{
    if (Coordinator)
        return Coordinator->GetWorld();
    return nullptr;
}

void UCheckCondition::Reset()
{
    bCompleted = false;
    bApproved = false;
    CurrentTransactionId.Invalidate();
}

// ===== ЛОГИРОВАНИЕ =====

int32 UCheckCondition::CurrentDepth = 0;
bool UCheckCondition::bEnableVerboseLogging = false; // по умолчанию выключено

static FAutoConsoleVariableRef CVarCheckConditionLogging(
    TEXT("CheckCondition.Logging"),
    UCheckCondition::bEnableVerboseLogging,
    TEXT("Enable verbose logging for CheckCondition (0=off, 1=on)")
);

void UCheckCondition::LogVerbose(const FString& Message, bool bIsStart)
{
    if (!bEnableVerboseLogging) return;

    // Формируем отступ с вертикальными линиями для каждого уровня глубины
    FString Indent;
    for (int32 i = 0; i < CurrentDepth; ++i)
    {
        Indent += TEXT("│ ");
    }

    FString Prefix = bIsStart ? TEXT("┌─ ") : TEXT("└─ ");
    FString DisplayMessage = Message;

    // Заменяем текстовые true/false на символы
    DisplayMessage = DisplayMessage.Replace(TEXT("→ true"), TEXT("→ ✅"));
    DisplayMessage = DisplayMessage.Replace(TEXT("→ false"), TEXT("→ ❌"));
    DisplayMessage = DisplayMessage.Replace(TEXT("(true)"), TEXT("(✅)"));
    DisplayMessage = DisplayMessage.Replace(TEXT("(false)"), TEXT("(❌)"));
    DisplayMessage = DisplayMessage.Replace(TEXT(" true"), TEXT(" ✅"));
    DisplayMessage = DisplayMessage.Replace(TEXT(" false"), TEXT(" ❌"));

    UE_LOG(LogTemp, Log, TEXT("%s%s%s %s"), *Indent, *Prefix, *GetDescription(), *DisplayMessage);
}

#if WITH_EDITOR
void UCheckCondition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    DebugDescription = GetDescription();
}
#endif