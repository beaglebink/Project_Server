#pragma once

#include "CoreMinimal.h"
#include "SpawnGroupLocationPayloadBase.h"
#include "SpawnGroupTypes.h"
#include "GameplayTagContainer.h"
#include "SpawnGroupConfigurePayload.generated.h"

class USpawnGroupAsset;

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupConfigurePayload : public USpawnGroupLocationPayloadBase
{
    GENERATED_BODY()

public:
    // ГЛАВНОЕ ПОЛЕ: Сам ассет, который мы передаем спавнеру
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure")
    TObjectPtr<USpawnGroupAsset> NewSpawnGroupAsset = nullptr;

    // ---- Ниже минимальные настройки самого актора-спавнера (опционально) ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Runtime")
    bool bApplyStatus = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Runtime", meta = (EditCondition = "bApplyStatus"))
    ESpawnGroupStatus Status = ESpawnGroupStatus::Inactive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Runtime")
    bool bApplyBlockNewSpawn = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Runtime", meta = (EditCondition = "bApplyBlockNewSpawn"))
    bool bBlockNewSpawn = false;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Configure")
    USpawnGroupConfigurePayload* Setup(const FGuid& InGroupId) { GroupId = InGroupId; return this; }
};