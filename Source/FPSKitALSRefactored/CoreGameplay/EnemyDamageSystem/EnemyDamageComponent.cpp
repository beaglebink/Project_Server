#include "EnemyDamageComponent.h"
#include "EnemyDamageConfig.h"
#include "DamageProcessingEffect.h"
#include "Engine/World.h"
#include "TimerManager.h"

UEnemyDamageComponent::UEnemyDamageComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UEnemyDamageComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeFromConfig(); // теперь всегда вызывается, независимо от наличия Config
}

void UEnemyDamageComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(HealthRegenTimer);
        World->GetTimerManager().ClearTimer(ReserveRegenTimer);
        World->GetTimerManager().ClearTimer(StaggerCooldownTimer);
    }
}

void UEnemyDamageComponent::SetConfig(UEnemyDamageConfig* InConfig)
{
    if (InConfig != Config)
    {
        Config = InConfig;
        if (GetWorld() && GetWorld()->HasBegunPlay())
        {
            InitializeFromConfig();
        }
    }
}

void UEnemyDamageComponent::InitializeFromConfig()
{
    // Сброс базового состояния
    bIsDead = false;
    bStaggerOnCooldown = false;
    RecentAttackIDs.Empty();
    LastHealthDamageTime = 0.0f;

    // Остановка старых таймеров
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(HealthRegenTimer);
        World->GetTimerManager().ClearTimer(ReserveRegenTimer);
        World->GetTimerManager().ClearTimer(StaggerCooldownTimer);
    }

    if (Config)
    {
        // Инициализация из конфига
        Health = FMath::Min(Config->InitialHealth, Config->MaxHealth);
        CurrentReserves.Empty();
        LastReserveDamageTimes.Empty();
        for (const FDefenseLayer& Layer : Config->DefenseLayers)
        {
            if (Layer.LayerType == EDefenseLayerType::Reserve)
            {
                CurrentReserves.Add(FMath::Min(Layer.InitialReserve, Layer.MaxReserve));
            }
            else
            {
                CurrentReserves.Add(0.0f);
            }
            LastReserveDamageTimes.Add(0.0f);
        }

        // Сортировка зон здоровья
        Config->HealthZones.Sort([](const FHealthZoneDefinition& A, const FHealthZoneDefinition& B) {
            return A.UpperBound > B.UpperBound;
            });
        UpdateHealthZone();

        // Запуск регенерации
        if (World)
        {
            if (Config->HealthRegen.RegenRatePerSecond > 0.0f)
            {
                World->GetTimerManager().SetTimer(HealthRegenTimer, this, &UEnemyDamageComponent::ApplyHealthRegen, 0.1f, true);
            }
            if (Config->DefenseLayers.ContainsByPredicate([](const FDefenseLayer& L) {
                return L.LayerType == EDefenseLayerType::Reserve && L.ReserveRegen.RegenRatePerSecond > 0.0f;
                }))
            {
                World->GetTimerManager().SetTimer(ReserveRegenTimer, this, &UEnemyDamageComponent::ApplyReserveRegen, 0.1f, true);
            }
        }
    }
    else
    {
        // Без конфига – только здоровье, без защиты, зон и регенерации
        Health = FMath::Max(0.0f, InitialHealth);
        CurrentReserves.Empty();
        LastReserveDamageTimes.Empty();
        CurrentHealthZoneTag = NAME_None;
        // Таймеры не запускаем
    }
}

FDamageResult UEnemyDamageComponent::TakeDamage(const FAttackDamageInfo& AttackInfo)
{
    FDamageResult Result;
    if (bIsDead)
        return Result;

    UWorld* World = GetWorld();
    if (!World)
        return Result;

    // 1. Проверка дубликата атаки
    if (RecentAttackIDs.Contains(AttackInfo.AttackID))
        return Result;
    RecentAttackIDs.Add(AttackInfo.AttackID);
    if (RecentAttackIDs.Num() > MaxRecentAttacks)
        RecentAttackIDs.RemoveAt(0);

    // Сохраняем состояние до обработки
    float OldHealth = Health;
    TArray<float> OldReserves = CurrentReserves;

    // 2. Расчёт модифицированного урона
    float ModifiedDamage = AttackInfo.BaseDamage * AttackInfo.GunConditionModifier * AttackInfo.ClothingModifier;
    Result.IncomingDamage = ModifiedDamage;

    // 3. Зона попадания (только если есть конфиг)
    float HitZoneMult = 1.0f;
    if (Config)
    {
        FName HitBone = AttackInfo.HitResult.BoneName;
        if (!HitBone.IsNone())
        {
            for (const FHitZoneDefinition& Zone : Config->HitZones)
            {
                if (Zone.BoneNames.Contains(HitBone))
                {
                    HitZoneMult = Zone.DamageMultiplier;
                    break;
                }
            }
        }
    }
    Result.HitZoneMultiplier = HitZoneMult;
    float DamageAfterZone = ModifiedDamage * HitZoneMult;
    Result.DamageAfterZone = DamageAfterZone;

    float AttackStrength = DamageAfterZone;
    Result.AttackStrength = AttackStrength;

    // 4. Подготовка контекста (если есть конфиг и эффекты)
    float RemainingDamage = DamageAfterZone;
    if (Config)
    {
        FDamageProcessingContext Context{ &RemainingDamage, Config, &CurrentReserves };
        if (AttackInfo.OptionalEnemyEffects)
        {
            AttackInfo.OptionalEnemyEffects->ModifyDamageProcessing(Context);
        }
    }
    // Если конфига нет, RemainingDamage остаётся равным DamageAfterZone

    // 5. Применение слоёв защиты (только если есть конфиг)
    float TotalDamageDealt = 0.0f;
    if (Config)
    {
        Result.LayerAbsorbedDamage.SetNum(Config->DefenseLayers.Num());
        Result.NewReserves.SetNum(Config->DefenseLayers.Num());
        Result.ReserveDepleted.SetNum(Config->DefenseLayers.Num());

        for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
        {
            Result.LayerAbsorbedDamage[i] = 0.0f;
            Result.ReserveDepleted[i] = false;

            const FDefenseLayer& Layer = Config->DefenseLayers[i];
            if (Layer.LayerType == EDefenseLayerType::Resistance)
            {
                float Absorbed = RemainingDamage * (1.0f - Layer.ResistanceMultiplier);
                Result.LayerAbsorbedDamage[i] = Absorbed;
                RemainingDamage *= Layer.ResistanceMultiplier;
            }
            else if (Layer.LayerType == EDefenseLayerType::Reserve)
            {
                float& Reserve = CurrentReserves[i];
                if (RemainingDamage <= Reserve)
                {
                    Result.LayerAbsorbedDamage[i] = RemainingDamage;
                    Reserve -= RemainingDamage;
                    TotalDamageDealt += RemainingDamage;
                    RemainingDamage = 0.0f;
                }
                else
                {
                    Result.LayerAbsorbedDamage[i] = Reserve;
                    TotalDamageDealt += Reserve;
                    RemainingDamage -= Reserve;
                    Reserve = 0.0f;
                    Result.ReserveDepleted[i] = true;
                }
                Result.NewReserves[i] = Reserve;

                OnReserveChanged.Broadcast(Layer.LayerTag, Reserve);
                if (Result.ReserveDepleted[i])
                {
                    OnReserveDepleted.Broadcast(Layer.LayerTag);
                }

                if (Result.LayerAbsorbedDamage[i] > 0.0f)
                {
                    LastReserveDamageTimes[i] = World->GetTimeSeconds();
                }
            }
        }
    }
    else
    {
        // Без конфига – нет слоёв защиты
        Result.LayerAbsorbedDamage.Empty();
        Result.NewReserves.Empty();
        Result.ReserveDepleted.Empty();
    }

    // 6. Пост-защитные эффекты (если есть конфиг и эффекты)
    float FinalHealthDamage = RemainingDamage;
    if (Config && AttackInfo.OptionalEnemyEffects)
    {
        FDamageProcessingContext Context{ &RemainingDamage, Config, &CurrentReserves };
        AttackInfo.OptionalEnemyEffects->PostDefenseProcessing(Context, FinalHealthDamage);
    }
    Result.FinalHealthDamage = FinalHealthDamage;

    // 7. Вычитание здоровья
    Health = FMath::Max(0.0f, Health - FinalHealthDamage);
    float HealthDelta = Health - OldHealth;
    Result.NewHealth = Health;

    // Добавляем урон по здоровью в общий реальный урон
    TotalDamageDealt += FinalHealthDamage;
    Result.TotalDamageDealt = TotalDamageDealt;

    // Событие изменения здоровья
    if (!FMath::IsNearlyEqual(HealthDelta, 0.0f, 0.001f))
    {
        OnHealthChanged.Broadcast(Health, HealthDelta);
    }
    if (FinalHealthDamage > 0.0f)
    {
        LastHealthDamageTime = World->GetTimeSeconds();
    }

    // 8. Определение zero‑damage
    bool bHealthChanged = !FMath::IsNearlyEqual(OldHealth, Health, 0.001f);
    bool bAnyReserveChanged = false;
    if (Config)
    {
        for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
        {
            if (Config->DefenseLayers[i].LayerType == EDefenseLayerType::Reserve)
            {
                if (!FMath::IsNearlyEqual(OldReserves[i], CurrentReserves[i], 0.001f))
                {
                    bAnyReserveChanged = true;
                    break;
                }
            }
        }
    }
    Result.bZeroDamage = !bHealthChanged && !bAnyReserveChanged;

    // 9. Стаггер (только если есть конфиг, не zero‑damage и не на кулдауне)
    if (Config && !bStaggerOnCooldown && !Result.bZeroDamage)
    {
        float StaggerDamage = 0.0f;
        switch (Config->StaggerInput)
        {
        case EStaggerInputType::UseFinalHealthDamage:
            StaggerDamage = FinalHealthDamage;
            break;
        case EStaggerInputType::UseTotalDamageDealt:
            StaggerDamage = TotalDamageDealt;
            break;
        case EStaggerInputType::UseAttackStrength:
            StaggerDamage = AttackStrength;
            break;
        default:
            StaggerDamage = TotalDamageDealt;
        }

        float Chance = GetStaggerChance(StaggerDamage);
        if (FMath::FRand() < Chance)
        {
            Result.bStaggerTriggered = true;
            bStaggerOnCooldown = true;
            OnStaggered.Broadcast();
            World->GetTimerManager().SetTimer(StaggerCooldownTimer, this, &UEnemyDamageComponent::OnStaggerCooldownExpired, Config->StaggerCooldown, false);
        }
    }

    // 10. Зона здоровья (только если есть конфиг)
    if (Config)
    {
        FName OldZone = CurrentHealthZoneTag;
        UpdateHealthZone();
        Result.CurrentHealthZoneTag = CurrentHealthZoneTag;
        Result.bHealthZoneChanged = (OldZone != CurrentHealthZoneTag);
        if (Result.bHealthZoneChanged)
        {
            OnHealthZoneChanged.Broadcast(CurrentHealthZoneTag, OldZone);
        }
    }
    else
    {
        Result.CurrentHealthZoneTag = NAME_None;
        Result.bHealthZoneChanged = false;
    }

    // 11. Проверка смерти
    if (Health <= 0.0f && !bIsDead)
    {
        bIsDead = true;
        Result.bKilled = true;
        OnDeath.Broadcast();
        // Останавливаем регенерацию
        if (World)
        {
            World->GetTimerManager().ClearTimer(HealthRegenTimer);
            World->GetTimerManager().ClearTimer(ReserveRegenTimer);
        }
    }

    // 12. Финальное событие
    OnDamageTaken.Broadcast(Result);

    return Result;
}

void UEnemyDamageComponent::UpdateHealthZone()
{
    if (!Config || Config->HealthZones.Num() == 0)
    {
        CurrentHealthZoneTag = NAME_None;
        return;
    }

    for (const FHealthZoneDefinition& Zone : Config->HealthZones)
    {
        if (Health > Zone.LowerBound && Health <= Zone.UpperBound)
        {
            CurrentHealthZoneTag = Zone.ZoneTag;
            return;
        }
    }
    CurrentHealthZoneTag = Config->HealthZones.Last().ZoneTag;
}

void UEnemyDamageComponent::ApplyHealthRegen()
{
    if (!Config || bIsDead) return;
    UWorld* World = GetWorld();
    if (!World) return;

    const FRegenerationParams& Params = Config->HealthRegen;
    if (Params.bInterruptOnDamage && (World->GetTimeSeconds() - LastHealthDamageTime) < Params.RegenDelayAfterDamage)
        return;

    float Delta = Params.RegenRatePerSecond * 0.1f;
    float OldHealth = Health;
    Health = FMath::Min(Config->MaxHealth, Health + Delta);
    float ActualDelta = Health - OldHealth;
    if (ActualDelta > 0.0f)
    {
        OnHealthChanged.Broadcast(Health, ActualDelta);
        FName OldZone = CurrentHealthZoneTag;
        UpdateHealthZone();
        if (OldZone != CurrentHealthZoneTag)
        {
            OnHealthZoneChanged.Broadcast(CurrentHealthZoneTag, OldZone);
        }
    }
}

void UEnemyDamageComponent::ApplyReserveRegen()
{
    if (!Config || bIsDead) return;
    UWorld* World = GetWorld();
    if (!World) return;

    for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
    {
        const FDefenseLayer& Layer = Config->DefenseLayers[i];
        if (Layer.LayerType != EDefenseLayerType::Reserve) continue;
        const FRegenerationParams& Params = Layer.ReserveRegen;
        if (Params.RegenRatePerSecond <= 0.0f) continue;

        if (Params.bInterruptOnDamage && (World->GetTimeSeconds() - LastReserveDamageTimes[i]) < Params.RegenDelayAfterDamage)
            continue;

        float& Reserve = CurrentReserves[i];
        float OldReserve = Reserve;
        Reserve = FMath::Min(Layer.MaxReserve, Reserve + Params.RegenRatePerSecond * 0.1f);
        float ActualDelta = Reserve - OldReserve;
        if (ActualDelta > 0.0f)
        {
            OnReserveChanged.Broadcast(Layer.LayerTag, Reserve);
        }
    }
}

void UEnemyDamageComponent::OnStaggerCooldownExpired()
{
    bStaggerOnCooldown = false;
}

float UEnemyDamageComponent::GetStaggerChance(float DamageValue) const
{
    if (!Config) return 0.0f;
    return FMath::Clamp(Config->BaseStaggerChance + DamageValue * Config->StaggerSusceptibility, 0.0f, 1.0f);
}

float UEnemyDamageComponent::GetReserveForLayer(int32 Index) const
{
    if (Config && CurrentReserves.IsValidIndex(Index) && Config->DefenseLayers.IsValidIndex(Index) && Config->DefenseLayers[Index].LayerType == EDefenseLayerType::Reserve)
    {
        return CurrentReserves[Index];
    }
    return 0.0f;
}

void UEnemyDamageComponent::ResetState()
{
    InitializeFromConfig();
}