#pragma once

#include "CoreMinimal.h"
#include "SpawnGroupLocationPayloadBase.h"
#include "SpawnGroupTypes.h"
#include "GameplayTagContainer.h"
#include "SpawnGroupConfigurePayload.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupConfigurePayload : public USpawnGroupLocationPayloadBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure") bool bApplyStatus = false;
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure") ESpawnGroupStatus Status = ESpawnGroupStatus::Inactive;

    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure") bool bApplyBlockNewSpawn = false;
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure") bool bBlockNewSpawn = false;

    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure") bool bApplyIsStoreSpawnParameters = false;
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure") bool bIsStoreSpawnParameters = false;

    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure") bool bApplyIsFullRespawn = false;
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure") bool bIsFullRespawn = false;

    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure") bool bApplyEnemyTags = false;
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure") TArray<FName> EnemyTags;

    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure") bool bApplyEnemyGameplayTags = false;
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup|Configure") FGameplayTagContainer EnemyGameplayTags;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Configure")
    USpawnGroupConfigurePayload* Setup(const FGuid& InGroupId) { GroupId = InGroupId; return this; }
};