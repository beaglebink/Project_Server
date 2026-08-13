// EnemyDamageComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyDamageTypes.h"
#include "DamageProcessing.h"
#include "Delegates/Delegate.h"
#include "EnemyDamageComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageTaken, const FDamageResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyHealthChanged, float, NewHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReserveChanged, FName, LayerTag, float, NewReserve);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReserveDepleted, FName, LayerTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaggered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStaggerCooldownEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthZoneChanged, FName, NewZone, FName, OldZone);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDeath, AActor*, DeathActor, FName, DeathTag);
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UEnemyDamageComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnemyDamageComponent();

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage")
    FDamageResult TakeDamage(const FAttackDamageInfo& AttackInfo);

    // События
    UPROPERTY(BlueprintAssignable, Category = "Enemy Damage")
    FOnDamageTaken OnDamageTaken;

    UPROPERTY(BlueprintAssignable, Category = "Enemy Damage")           
    FOnEnemyHealthChanged OnHealthChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "Enemy Damage")
    FOnReserveChanged OnReserveChanged;

	UPROPERTY(BlueprintAssignable, Category = "Enemy Damage")
    FOnReserveDepleted OnReserveDepleted;

    UPROPERTY(BlueprintAssignable, Category = "Enemy Damage")
    FOnStaggered OnStaggered;

    UPROPERTY(BlueprintAssignable, Category = "Enemy Damage")
    FOnStaggerCooldownEnded OnStaggerCooldownEnded;

    UPROPERTY(BlueprintAssignable, Category = "Enemy Damage")
    FOnHealthZoneChanged OnHealthZoneChanged;

    UPROPERTY(BlueprintAssignable, Category = "Enemy Damage")
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
    float CurrentStaggerChance = 0;

    void InitializeFromConfig();
    void UpdateHealthZone();
    void ApplyHealthRegen();
    void ApplyReserveRegen();
    void OnStaggerCooldownExpired();
    float GetStaggerChance(float DamageValue) const;
    void DebugLogDamage(const FDamageResult& Result, float ModifiedDamage, float HitZoneMult, float DamageAfterZone, float FinalHealthDamage, float HealthDelta, bool bAnyReserveChanged);
};