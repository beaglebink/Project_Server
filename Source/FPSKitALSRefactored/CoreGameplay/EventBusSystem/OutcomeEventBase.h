// OutcomeEventBase.h
#pragma once

#include "CoreMinimal.h"
#include "Outcome.h"
#include "OutcomePayload.h"
#include "GameplayTagContainer.h"          // ← NEW
#include "OutcomeEventBase.generated.h"

USTRUCT(BlueprintType)
struct FOutcomeEventBase
{
    GENERATED_BODY()

    // ===== FILTER FIELDS =====
    UPROPERTY(BlueprintReadWrite, Category = "Outcome") EOutcomeType       OutcomeType = EOutcomeType::Default;
    UPROPERTY(BlueprintReadWrite, Category = "Outcome") EOutcomeMission    OutcomeMission = EOutcomeMission::Default;
    UPROPERTY(BlueprintReadWrite, Category = "Outcome") EOutcomeActor      OutcomeActor = EOutcomeActor::Default;
    UPROPERTY(BlueprintReadWrite, Category = "Outcome") EOutcomeInventory  OutcomeInventory = EOutcomeInventory::Default;
    UPROPERTY(BlueprintReadWrite, Category = "Outcome") EOutcomeTerminal   OutcomeTerminal = EOutcomeTerminal::Default;
    UPROPERTY(BlueprintReadWrite, Category = "Outcome") EOutcomeInterior   OutcomeInterior = EOutcomeInterior::Default;
    UPROPERTY(BlueprintReadWrite, Category = "Outcome") EOutcomeSpawnGroup OutcomeSpawnGroup = EOutcomeSpawnGroup::Default;
    UPROPERTY(BlueprintReadWrite, Category = "Outcome") EOutcomeWorldState OutcomeWorldState = EOutcomeWorldState::Default;
    UPROPERTY(BlueprintReadWrite, Category = "Outcome") EOutcomeChore      OutcomeChore = EOutcomeChore::Default;

    // ===== optional payload =====
    UPROPERTY(BlueprintReadWrite, Category = "Outcome")
    TObjectPtr<UOutcomePayload> Payload = nullptr;

    // ===== теги для ScenarioManager =====
    // Большинство команд и отчётов оставляют пустыми; заполняются только там,
    // где реально нужен сдвиг сценария.
    UPROPERTY(BlueprintReadWrite, Category = "Outcome|Tags")
    TArray<FGameplayTag> TagsToAdd;

    UPROPERTY(BlueprintReadWrite, Category = "Outcome|Tags")
    TArray<FGameplayTag> TagsToRemove;
};