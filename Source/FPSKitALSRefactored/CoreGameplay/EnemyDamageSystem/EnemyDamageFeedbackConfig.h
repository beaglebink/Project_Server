#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "EnemyDamageFeedbackConfig.generated.h"

UCLASS(BlueprintType)
class UEnemyDamageFeedbackConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Visual")
    TSoftObjectPtr<UNiagaraSystem> ImpactEffect;

    UPROPERTY(EditAnywhere, Category = "Visual")
    TSoftObjectPtr<UNiagaraSystem> ZeroDamageEffect;

    UPROPERTY(EditAnywhere, Category = "Visual")
    TSoftObjectPtr<UNiagaraSystem> ReserveDamageEffect;

    UPROPERTY(EditAnywhere, Category = "Visual")
    TSoftObjectPtr<UNiagaraSystem> ReserveDepletedEffect;

    UPROPERTY(EditAnywhere, Category = "Visual")
    TSoftObjectPtr<UNiagaraSystem> StaggerEffect;

    UPROPERTY(EditAnywhere, Category = "Visual")
    TSoftObjectPtr<UNiagaraSystem> DeathEffect;

    UPROPERTY(EditAnywhere, Category = "Audio")
    TSoftObjectPtr<USoundBase> HitSound;

    UPROPERTY(EditAnywhere, Category = "Audio")
    TSoftObjectPtr<USoundBase> DeathSound;
};