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
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSuicide);

USTRUCT(BlueprintType)
struct FEnemyDamageSaveData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    float Health = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    TArray<float> CurrentReserves;

    UPROPERTY(BlueprintReadWrite)
    bool bIsDead = false;

    UPROPERTY(BlueprintReadWrite)
    bool bStaggerOnCooldown = false;

    UPROPERTY(BlueprintReadWrite)
    FName CurrentHealthZoneTag;

    UPROPERTY(BlueprintReadWrite)
    float LastHealthDamageTime = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    TArray<float> LastReserveDamageTimes;

    UPROPERTY(BlueprintReadWrite)
    float MaxHealth = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    TArray<float> RuntimeMaxReserves;

    UPROPERTY(BlueprintReadWrite)
    TArray<float> RuntimeResistanceMultipliers;

    UPROPERTY(BlueprintReadWrite)
    TArray<FRegenerationParams> RuntimeReserveRegenParams;

    UPROPERTY(BlueprintReadWrite)
    FRegenerationParams RuntimeHealthRegenParams;

    UPROPERTY(BlueprintReadWrite)
    TArray<FHealthZoneDefinition> RuntimeHealthZones;

    UPROPERTY(BlueprintReadWrite)
    bool bHealthRegenEnabled = true;

    UPROPERTY(BlueprintReadWrite)
    TArray<bool> bReserveRegenEnabled;

    UPROPERTY(BlueprintReadWrite)
    float RuntimeBaseStaggerChance = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    float RuntimeStaggerSusceptibility = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    float RuntimeStaggerCooldown = 2.0f;

    UPROPERTY(BlueprintReadWrite)
    EStaggerInputType RuntimeStaggerInputType = EStaggerInputType::UseTotalDamageDealt;

    UPROPERTY(BlueprintReadWrite)
    FName CurrentHitZoneName = NAME_None;
};

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
	FOnSuicide OnSuicide;

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

    // ---- Управление здоровьем ----
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Health")
    void Heal(float Amount);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Health")
    void HealToMax(bool bInstant = true);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Health")
    void SetHealth(float NewHealth);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Health")
    void SetMaxHealth(float NewMaxHealth);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Enemy Damage|Health")
    bool GetIsDead() const { return bIsDead; }

    // ---- Управление резервами ----
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Reserves")
    void RestoreReserve(int32 LayerIndex, float Amount);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Reserves")
    void RestoreReserveToMax(int32 LayerIndex);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Reserves")
    void SetReserve(int32 LayerIndex, float NewValue);

    // ---- Управление сопротивлением ----
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Defense")
    void SetResistanceMultiplier(int32 LayerIndex, float NewMultiplier);

    // ---- Управление регенерацией здоровья ----
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Regeneration")
    void SetHealthRegenEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Regeneration")
    void SetHealthRegenRatePerSecond(float Rate);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Regeneration")
    void SetHealthRegenDelay(float Delay);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Regeneration")
    void SetHealthRegenInterruptOnDamage(bool bInterrupt);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Health")
    void SetHealthZones(const TArray<FHealthZoneDefinition>& NewZones);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Regeneration")
    void SetHealthRegenWhileStaggered(bool bAllow);

    // ---- Управление регенерацией резервов ----
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Regeneration")
    void SetAllReserveRegenEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Regeneration")
    void SetReserveRegenEnabled(int32 LayerIndex, bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Reserves")
    void SetMaxReserve(int32 LayerIndex, float NewMaxReserve);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Regeneration")
    void SetReserveRegenPerSecond(int32 LayerIndex, float Rate);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Regeneration")
    void SetReserveRegenDelay(int32 LayerIndex, float Delay);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Regeneration")
    void SetReserveRegenInterruptOnDamage(int32 LayerIndex, bool bInterrupt);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Regeneration")
    void SetReserveRegenWhileStaggered(int32 LayerIndex, bool bAllow);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Regeneration")
    void SetAllReserveRegenWhileStaggered(bool bAllow);

    // ---- Управление стаггером ----
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Stagger")
    void SetStaggerParams(float BaseChance, float Susceptibility, float Cooldown, EStaggerInputType InputType);

    // ---- Принудительная смерть ----
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Death")
    void Kill();

    // ---- Сериализация в строку ----
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Save")
    FString SerializeToString() const;

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Save")
    bool DeserializeFromString(const FString& Data);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Damage")
    float InitialHealth = 100.0f;

private:
    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "Enemy Damage", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UEnemyDamageConfig> Config;

    UPROPERTY(SaveGame)
    float Health = 100.0f;

    UPROPERTY(SaveGame)
    TArray<float> CurrentReserves;

    UPROPERTY(SaveGame)
    bool bIsDead = false;

    UPROPERTY(SaveGame)
    bool bStaggerOnCooldown = false;

    UPROPERTY(SaveGame)
    FName CurrentHealthZoneTag;

    UPROPERTY(SaveGame)
    FTimerHandle HealthRegenTimer;

    UPROPERTY(SaveGame)
    FTimerHandle ReserveRegenTimer;

    UPROPERTY(SaveGame)
    FTimerHandle StaggerCooldownTimer;

    UPROPERTY(SaveGame)
    TArray<FGuid> RecentAttackIDs;

    static constexpr int32 MaxRecentAttacks = 30;

    UPROPERTY(SaveGame)
    float LastHealthDamageTime = 0.0f;

    UPROPERTY(SaveGame)
    TArray<float> LastReserveDamageTimes;

    UPROPERTY(SaveGame)
    float CurrentStaggerChance = 0;

    UPROPERTY(SaveGame)
    float MaxHealth = 0;

    UPROPERTY(SaveGame)
	FName CurrentHitZoneName = NAME_None;

    UPROPERTY(SaveGame)
    TArray<float> RuntimeMaxReserves;

    // Runtime-копии параметров для защиты (инициализируются из Config)
    UPROPERTY(SaveGame)
    TArray<float> RuntimeResistanceMultipliers;

    UPROPERTY(SaveGame)
    TArray<FRegenerationParams> RuntimeReserveRegenParams;
    //TArray<float> RuntimeReserveDamageMultipliers;

    UPROPERTY(SaveGame)
    FRegenerationParams RuntimeHealthRegenParams;

    // Runtime-копия зон здоровья (масштабируется при изменении MaxHealth)
    UPROPERTY(SaveGame)
    TArray<FHealthZoneDefinition> RuntimeHealthZones;

    // Флаги включения регенерации

    UPROPERTY(SaveGame)
    bool bHealthRegenEnabled = true;

    UPROPERTY(SaveGame)
    TArray<bool> bReserveRegenEnabled;

    // Параметры стаггера (runtime)
    UPROPERTY(SaveGame)
    float RuntimeBaseStaggerChance = 0.0f;

    UPROPERTY(SaveGame)
    float RuntimeStaggerSusceptibility = 0.0f;

    UPROPERTY(SaveGame)
    float RuntimeStaggerCooldown = 2.0f;

    UPROPERTY(SaveGame)
    EStaggerInputType RuntimeStaggerInputType = EStaggerInputType::UseTotalDamageDealt;

    void InitializeFromConfig();
	void CheckHealthZone();
    void UpdateHealthZone();
    void ApplyHealthRegen();
    void ApplyReserveRegen();
    void OnStaggerCooldownExpired();
    float GetStaggerChance(float DamageValue) const;
    FName PredictHealthZone(float HealthValue) const;
    
    // ---- Сохранение/загрузка состояния ----
    FEnemyDamageSaveData SaveState() const;
    void LoadState(const FEnemyDamageSaveData& SaveData);

    void DebugLogDamage(const FDamageResult& Result, float ModifiedDamage, float HitZoneMult, float DamageAfterZone, float FinalHealthDamage, float HealthDelta, bool bAnyReserveChanged);
};