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
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHealthZoneChanged, FName NewZone, FName OldZone);
DECLARE_MULTICAST_DELEGATE(FOnDeath);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UEnemyDamageComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEnemyDamageComponent();

    // Основной метод получения урона
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage")
    FDamageResult TakeDamage(const FAttackDamageInfo& AttackInfo);

    // События
    FOnDamageTaken OnDamageTaken;
    FOnEnemyHealthChanged OnHealthChanged;
    FOnReserveChanged OnReserveChanged;
    FOnReserveDepleted OnReserveDepleted;
    FOnStaggered OnStaggered;
    FOnHealthZoneChanged OnHealthZoneChanged;
    FOnDeath OnDeath;

    // Геттеры состояния
    UFUNCTION(BlueprintPure)
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintPure)
    float GetReserveForLayer(int32 Index) const;

    UFUNCTION(BlueprintPure)
    FName GetCurrentHealthZone() const { return CurrentHealthZoneTag; }

    UFUNCTION(BlueprintPure)
    bool IsDead() const { return bIsDead; }

    // Установка конфига (можно вызывать из кода или BP)
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage")
    void SetConfig(class UEnemyDamageConfig* InConfig);

    // Сброс состояния (например, для респавна)
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage")
    void ResetState();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Начальное здоровье (используется, если конфиг не задан)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Damage")
    float InitialHealth = 100.0f;

private:
    // Конфиг – редактируемый в редакторе, может быть nullptr
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy Damage", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UEnemyDamageConfig> Config;

    float Health = 100.0f;
    TArray<float> CurrentReserves; // соответствуют слоям Reserve
    bool bIsDead = false;
    bool bStaggerOnCooldown = false;
    FName CurrentHealthZoneTag;

    // Таймеры
    FTimerHandle HealthRegenTimer;
    FTimerHandle ReserveRegenTimer;
    FTimerHandle StaggerCooldownTimer;

    // Временная блокировка повторов атак
    TArray<FGuid> RecentAttackIDs;
    static constexpr int32 MaxRecentAttacks = 30;

    // Время последнего получения урона (для регенерации)
    float LastHealthDamageTime = 0.0f;
    TArray<float> LastReserveDamageTimes; // по слоям

    // Методы
    void InitializeFromConfig();
    void UpdateHealthZone();
    void ApplyHealthRegen();
    void ApplyReserveRegen();
    void OnStaggerCooldownExpired();
    float GetStaggerChance(float DamageValue) const;
};