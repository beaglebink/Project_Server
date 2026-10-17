// SpawnGroupConfigurePayload.h
#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "SpawnGroupTypes.h"
#include "GameplayTagContainer.h"
#include "SpawnGroupConfigurePayload.generated.h"

/**
 * Команда конфигурации спавнера.
 * Каждое поле применяется только если соответствующий bApplyX == true.
 * Если спавнер ещё не загружен — payload кладётся в очередь и применяется
 * при его регистрации (или при загрузке сохранения).
 *
 * (Command for spawner configuration.
 *  Each field is applied only if the corresponding bApplyX == true.
 *  If the spawner is not yet loaded — payload is queued and applied
 *  when it registers, or on save load.)
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupConfigurePayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure")
    FGuid GroupId;

    // ---- Status ----
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure")
    bool bApplyStatus = false;
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure")
    ESpawnGroupStatus Status = ESpawnGroupStatus::Inactive;

    // ---- BlockNewSpawn ----
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure")
    bool bApplyBlockNewSpawn = false;
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure")
    bool bBlockNewSpawn = false;

    // ---- IsStoreSpawnParameters ----
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure")
    bool bApplyIsStoreSpawnParameters = false;
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure")
    bool bIsStoreSpawnParameters = false;

    // ---- IsFullRespawn ----
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure")
    bool bApplyIsFullRespawn = false;
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure")
    bool bIsFullRespawn = false;

    // ---- EnemyTags ----
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure")
    bool bApplyEnemyTags = false;
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure")
    TArray<FName> EnemyTags;

    // ---- EnemyGameplayTags ----
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure")
    bool bApplyEnemyGameplayTags = false;
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure")
    FGameplayTagContainer EnemyGameplayTags;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Configure")
    USpawnGroupConfigurePayload* Setup(const FGuid& InGroupId)
    {
        GroupId = InGroupId;
        return this;
    }
};