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
    // ---- Здоровье ----
    UPROPERTY(EditAnywhere, Category = "Health")
    float MaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, Category = "Health")
    float InitialHealth = 100.0f;

    // ---- Защита ----
    UPROPERTY(EditAnywhere, Category = "Defense")
    TArray<FDefenseLayer> DefenseLayers;

    // ---- Зоны здоровья ----
    UPROPERTY(EditAnywhere, Category = "Health Zones")
    TArray<FHealthZoneDefinition> HealthZones;

    // ---- Зоны попадания ----
    UPROPERTY(EditAnywhere, Category = "Hit Zones")
    TArray<FHitZoneDefinition> HitZones;

    // ---- Стаггер ----
    UPROPERTY(EditAnywhere, Category = "Stagger")
    float BaseStaggerChance = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Stagger")
    float StaggerSusceptibility = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Stagger")
    float StaggerCooldown = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Stagger")
    EStaggerInputType StaggerInput = EStaggerInputType::UseTotalDamageDealt;

    // ---- Регенерация здоровья ----
    UPROPERTY(EditAnywhere, Category = "Health Regeneration")
    FRegenerationParams HealthRegen;

    // ---- Смерть ----
    // (ссылка на класс поведения смерти или тег)
    UPROPERTY(EditAnywhere, Category = "Death")
    FName DeathBehaviorTag;

    // ---- Валидация в редакторе ----
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    // Проверка пересечения костей в HitZones
    UFUNCTION()
    bool ValidateHitZones() const;

    // Проверка целостности зон здоровья
    UFUNCTION()
    bool ValidateHealthZones() const;
};