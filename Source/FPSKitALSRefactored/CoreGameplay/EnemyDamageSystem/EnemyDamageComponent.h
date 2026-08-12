// EnemyDamageComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyDamageTypes.h"
#include "DamageProcessing.h"
#include "EnemyDamageComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDamageTaken, const FDamageResult&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEnemyHealthChanged, float NewHealth, float Delta);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnReserveChanged, FName LayerTag, float NewReserve);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnReserveDepleted, FName LayerTag);
DECLARE_MULTICAST_DELEGATE(FOnStaggered);
DECLARE_MULTICAST_DELEGATE(FOnStaggerCooldownEnded);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHealthZoneChanged, FName NewZone, FName OldZone);
DECLARE_MULTICAST_DELEGATE(FOnDeath);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UEnemyDamageComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnemyDamageComponent();

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage")
    FDamageResult TakeDamage(const FAttackDamageInfo& AttackInfo);

    // События
    FOnDamageTaken OnDamageTaken;
    FOnEnemyHealthChanged OnHealthChanged;
    FOnReserveChanged OnReserveChanged;
    FOnReserveDepleted OnReserveDepleted;
    FOnStaggered OnStaggered;
    FOnStaggerCooldownEnded OnStaggerCooldownEnded;
    FOnHealthZoneChanged OnHealthZoneChanged;
    FOnDeath OnHealthDepleted;

    // Геттеры
    UFUNCTION(BlueprintPure)
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintPure)
    float GetReserveForLayer(int32 Index) const;

    UFUNCTION(BlueprintPure)
    FName GetCurrentHealthZone() const { return CurrentHealthZoneTag; }

    UFUNCTION(BlueprintPure)
    bool IsStaggerOnCooldown() const { return bStaggerOnCooldown; }

    UFUNCTION(BlueprintPure)
    bool IsDead() const { return bIsDead; }

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage")
    void SetConfig(class UEnemyDamageConfig* InConfig);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage")
    void ResetState();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Damage")
    float InitialHealth = 100.0f;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Damage", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UEnemyDamageConfig> Config;

    float Health = 100.0f;
    TArray<float> CurrentReserves;
    bool bIsDead = false;
    bool bStaggerOnCooldown = false;
    FName CurrentHealthZoneTag;

    FTimerHandle HealthRegenTimer;
    FTimerHandle ReserveRegenTimer;
    FTimerHandle StaggerCooldownTimer;

    TArray<FGuid> RecentAttackIDs;
    static constexpr int32 MaxRecentAttacks = 30;

    float LastHealthDamageTime = 0.0f;
    TArray<float> LastReserveDamageTimes;

    void InitializeFromConfig();
    void UpdateHealthZone();
    void ApplyHealthRegen();
    void ApplyReserveRegen();
    void OnStaggerCooldownExpired();
    float GetStaggerChance(float DamageValue) const;
    void DebugLogDamage(const FDamageResult& Result, float ModifiedDamage, float HitZoneMult, float DamageAfterZone, float FinalHealthDamage, float HealthDelta, bool bAnyReserveChanged);
};