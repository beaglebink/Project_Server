#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnemyDamageTypes.h"
#include "EnemyDamageConfig.generated.h"

UCLASS(BlueprintType)
class UEnemyDamageConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    // ---- Health ----
    // ---- Здоровье ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
    float MaxHealth = 100.0f;

    // ---- Defense ----
    // ---- Защита ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense")
    TArray<FDefenseLayer> DefenseLayers;

    // ---- Health Zones ----
    // ---- Зоны здоровья ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Zones")
    TArray<FHealthZoneDefinition> HealthZones;

    // ---- Hit Zones ----
    // ---- Зоны попадания ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Zones")
    TArray<FHitZoneDefinition> HitZones;

    // ---- Stagger ----
    // ---- Стаггер ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stagger")
    float BaseStaggerChance = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stagger")
    float StaggerSusceptibility = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stagger")
    float StaggerCooldown = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stagger")
    EStaggerInputType StaggerInput = EStaggerInputType::UseFinalHealthDamage;

    // ---- Health Regeneration ----
    // ---- Регенерация здоровья ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Regeneration")
    FRegenerationParams HealthRegen;

    // ---- Death ----
    // ---- Смерть ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Death")
    FName DeathBehaviorTag;

    // ---- Editor validation ----
    // ---- Валидация в редакторе ----
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    UFUNCTION(BlueprintCallable, Category = "Validation")
    bool ValidateHitZones() const;

    UFUNCTION(BlueprintCallable, Category = "Validation")
    bool ValidateHealthZones() const;
};