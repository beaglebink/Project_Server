#include "EnemyDamageComponent.h"
#include "EnemyDamageConfig.h"
#include "DamageProcessingEffect.h"
#include "Engine/World.h"
#include "TimerManager.h"

static TAutoConsoleVariable<int32> CVarEnemyDamageVerboseLogging(
    TEXT("EnemyDamage.VerboseLogging"),
    0,
    TEXT("Enable verbose logging for enemy damage processing.\n")
    TEXT("  0 = off (default)\n")
    TEXT("  1 = on - prints one log line per processed hit"),
    ECVF_Default);

UEnemyDamageComponent::UEnemyDamageComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UEnemyDamageComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeFromConfig();
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
    UWorld* World = GetWorld();

    bIsDead = false;
    bStaggerOnCooldown = false;
    RecentAttackIDs.Empty();
    LastHealthDamageTime = World ? World->GetTimeSeconds() : 0.0f;

    if (World)
    {
        World->GetTimerManager().ClearTimer(HealthRegenTimer);
        World->GetTimerManager().ClearTimer(ReserveRegenTimer);
        World->GetTimerManager().ClearTimer(StaggerCooldownTimer);
    }

    // Сброс runtime-полей
    RuntimeResistanceMultipliers.Empty();
    RuntimeReserveRegenParams.Empty();
    bReserveRegenEnabled.Empty();
    RuntimeHealthRegenParams = FRegenerationParams();
    bHealthRegenEnabled = true;
    RuntimeBaseStaggerChance = 0.0f;
    RuntimeStaggerSusceptibility = 0.0f;
    RuntimeStaggerCooldown = 2.0f;

    if (Config)
    {
        Health = FMath::Min(InitialHealth, Config->MaxHealth);
        MaxHealth = Config->MaxHealth;
        CurrentReserves.Empty();
        LastReserveDamageTimes.Empty();

        // Инициализация резервов и runtime-параметров
        for (const FDefenseLayer& Layer : Config->DefenseLayers)
        {
            if (Layer.LayerType == EDefenseLayerType::Reserve)
            {
                CurrentReserves.Add(FMath::Min(Layer.InitialReserve, Layer.MaxReserve));
                RuntimeResistanceMultipliers.Add(1.0f); // не используется для резерва
                RuntimeReserveRegenParams.Add(Layer.ReserveRegen);
                bReserveRegenEnabled.Add(true);
            }
            else // Resistance
            {
                CurrentReserves.Add(0.0f);
                RuntimeResistanceMultipliers.Add(Layer.ResistanceMultiplier);
                // для сопротивления нет регенерации
                RuntimeReserveRegenParams.Add(FRegenerationParams());
                bReserveRegenEnabled.Add(false);
            }
            LastReserveDamageTimes.Add(0.0f);
        }

        // Сортировка и проверка зон здоровья
        Config->HealthZones.Sort([](const FHealthZoneDefinition& A, const FHealthZoneDefinition& B) {
            return A.UpperBound > B.UpperBound;
            });
        
        RuntimeHealthZones = Config->HealthZones;
        UpdateHealthZone();
        CheckHealthZone();

        // Копируем параметры регенерации здоровья и стаггера
        RuntimeHealthRegenParams = Config->HealthRegen;
        RuntimeBaseStaggerChance = Config->BaseStaggerChance;
        RuntimeStaggerSusceptibility = Config->StaggerSusceptibility;
        
        //if(Config->StaggerCooldown > 0)
        RuntimeStaggerCooldown = Config->StaggerCooldown;

        // Запуск таймеров регенерации
        if (World)
        {
            if (bHealthRegenEnabled && RuntimeHealthRegenParams.RegenRatePerSecond > 0.0f)
            {
                World->GetTimerManager().SetTimer(HealthRegenTimer, this, &UEnemyDamageComponent::ApplyHealthRegen, 0.1f, true);
            }
            // Резервная регенерация – хотя бы один включённый резерв с положительной скоростью
            bool bAnyReserveRegen = false;
            for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
            {
                if (Config->DefenseLayers[i].LayerType == EDefenseLayerType::Reserve &&
                    bReserveRegenEnabled[i] &&
                    RuntimeReserveRegenParams[i].RegenRatePerSecond > 0.0f)
                {
                    bAnyReserveRegen = true;
                    break;
                }
            }
            if (bAnyReserveRegen)
            {
                World->GetTimerManager().SetTimer(ReserveRegenTimer, this, &UEnemyDamageComponent::ApplyReserveRegen, 0.1f, true);
            }
        }
    }
    else // Без конфига – только здоровье
    {
        Health = FMath::Max(0.0f, InitialHealth);
        MaxHealth = Health;
        CurrentReserves.Empty();
        LastReserveDamageTimes.Empty();
        CurrentHealthZoneTag = NAME_None;
        // таймеры не запускаются
    }
}

void UEnemyDamageComponent::CheckHealthZone()
{
    if (RuntimeHealthZones.Num() > 0)
    {
        if (!FMath::IsNearlyEqual(RuntimeHealthZones[0].UpperBound, MaxHealth, 0.001f))
        {
            UE_LOG(LogTemp, Error, TEXT("Health zone invalid: first zone UpperBound (%f) != MaxHealth (%f) in config %s"),
                RuntimeHealthZones[0].UpperBound, MaxHealth, *GetNameSafe(Config));
        }
        if (!FMath::IsNearlyEqual(RuntimeHealthZones.Last().LowerBound, 0.0f, 0.001f))
        {
            UE_LOG(LogTemp, Error, TEXT("Health zone invalid: last zone LowerBound (%f) != 0 in config %s"),
                RuntimeHealthZones.Last().LowerBound, *GetNameSafe(Config));
        }
        for (int32 i = 0; i < RuntimeHealthZones.Num() - 1; ++i)
        {
            if (!FMath::IsNearlyEqual(RuntimeHealthZones[i].LowerBound, RuntimeHealthZones[i + 1].UpperBound, 0.001f))
            {
                UE_LOG(LogTemp, Error, TEXT("Health zone gap/overlap between %s and %s in config %s (%s)"),
                    *RuntimeHealthZones[i].ZoneTag.ToString(), *RuntimeHealthZones[i + 1].ZoneTag.ToString(), *GetNameSafe(Config), *GetNameSafe(GetOwner()));
            }
        }
    }
}

FDamageResult UEnemyDamageComponent::TakeDamage(const FAttackDamageInfo& AttackInfo)
{
    FDamageResult Result;
    Result.MaxHealth = MaxHealth;
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

    CheckHealthZone(); // валидация зон (на случай изменения конфига)

    // Сохраняем состояние до обработки
    float OldHealth = Health;
    TArray<float> OldReserves = CurrentReserves;
    FName OldHealthZone = CurrentHealthZoneTag;

    if (AttackInfo.BaseDamage < 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Negative damage"));
        return Result;
    }

    // 2. Расчёт модифицированного урона
    float ModifiedDamage = AttackInfo.BaseDamage * AttackInfo.GunConditionModifier * AttackInfo.ClothingModifier;
    Result.IncomingDamage = ModifiedDamage;

    // 3. Зона попадания (кости + компоненты)
    float HitZoneMult = 1.0f;
    if (Config)
    {
        FName HitBone = AttackInfo.HitResult.BoneName;
        FName HitComponentName = AttackInfo.HitResult.Component.IsValid() ? AttackInfo.HitResult.Component->GetFName() : NAME_None;

        for (const FHitZoneDefinition& Zone : Config->HitZones)
        {
            bool bMatched = false;
            if (!HitBone.IsNone() && Zone.BoneNames.Contains(HitBone))
                bMatched = true;
            if (!bMatched && !HitComponentName.IsNone() && Zone.ComponentNames.Contains(HitComponentName))
                bMatched = true;

            if (bMatched)
            {
                HitZoneMult = Zone.DamageMultiplier;
                CurrentHitZoneName = Zone.ZoneName;
                break;
            }
        }
    }
    Result.HitZoneMultiplier = HitZoneMult;
    float DamageAfterZone = ModifiedDamage * HitZoneMult;
    Result.DamageAfterZone = DamageAfterZone;
    float AttackStrength = DamageAfterZone;
    Result.AttackStrength = AttackStrength;

    // 4. Контекст эффектов (Modify + ApplyDefense)
    float RemainingDamage = DamageAfterZone;
    TArray<float> ResistanceMods;
    TArray<float> ReserveDamageMods;
    TArray<bool> IgnoreLayer;
    float FinalHealthMod = 1.0f;
    float StaggerMod = 0.0f;
    bool bForceStagger = false;
    float bNewForceStaggerCooldown = 2.0f;

    if (Config)
    {
        int32 NumLayers = Config->DefenseLayers.Num();
        ResistanceMods.Init(1.0f, NumLayers);
        ReserveDamageMods.Init(1.0f, NumLayers);
        IgnoreLayer.Init(false, NumLayers);

        FDamageProcessingContext Context;
        Context.IncomingDamage = RemainingDamage;
        Context.Config = Config;
        Context.Reserves = CurrentReserves;
        Context.ResistanceMultipliers = ResistanceMods;
        Context.ReserveDamageMultipliers = ReserveDamageMods;
        Context.IgnoreLayer = IgnoreLayer;
        Context.FinalHealthDamage = 0.0f;
        Context.StaggerChanceModifier = 0.0f;
        Context.CurrentHealthZone = OldHealthZone;
        Context.PreviousHealthZone = OldHealthZone;
        Context.Owner = GetOwner();
        Context.Weapon = AttackInfo.DamageSource;
        Context.ForceStaggerCooldown = RuntimeStaggerCooldown;
        Context.Health = Health; // текущее здоровье до урона

        if (AttackInfo.OptionalEnemyEffects)
        {
            FPreDefenseOutput Out = AttackInfo.OptionalEnemyEffects->ModifyDamageProcessing(Context);
            RemainingDamage = Out.NewIncomingDamage;
            Context.IncomingDamage = RemainingDamage;
        }

        if (AttackInfo.OptionalEnemyEffects)
        {
            FDefenseOutput Out = AttackInfo.OptionalEnemyEffects->ApplyDefenseModifiers(Context);
            ResistanceMods = Out.NewModifiers.ResistanceMultipliers;
            ReserveDamageMods = Out.NewModifiers.ReserveDamageMultipliers;
            IgnoreLayer = Out.NewModifiers.IgnoreLayer;
            Context.ResistanceMultipliers = ResistanceMods;
            Context.ReserveDamageMultipliers = ReserveDamageMods;
            Context.IgnoreLayer = IgnoreLayer;
        }
    }

    // 5. Применение слоёв защиты (с учётом runtime-сопротивлений)
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

            if (IgnoreLayer.IsValidIndex(i) && IgnoreLayer[i])
                continue;
            if (!IgnoreLayer.IsValidIndex(i))
            {
                AActor* Owner = GetOwner();
                FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
                UE_LOG(LogTemp, Error, TEXT("Owner %s Incorrect IgnoreLayer array size (expected %d, got %d)"),
                    *OwnerName, Config->DefenseLayers.Num(), IgnoreLayer.Num());
                continue;
            }

            const FDefenseLayer& Layer = Config->DefenseLayers[i];
            if (Layer.LayerType == EDefenseLayerType::Resistance)
            {
                float EffectiveResistance = FMath::Clamp(RuntimeResistanceMultipliers[i] * ResistanceMods[i], 0.0f, 1.0f);
                float Absorbed = RemainingDamage * (1.0f - EffectiveResistance);
                Result.LayerAbsorbedDamage[i] = Absorbed;
                RemainingDamage *= EffectiveResistance;
            }
            else // Reserve
            {
                float& Reserve = CurrentReserves[i];
                float DamageToReserve = RemainingDamage * ReserveDamageMods[i];
                if (DamageToReserve <= 0.0f)
                    continue;

                if (DamageToReserve <= Reserve)
                {
                    Result.LayerAbsorbedDamage[i] = DamageToReserve;
                    Reserve -= DamageToReserve;
                    TotalDamageDealt += DamageToReserve;
                    RemainingDamage = 0.0f;
                }
                else
                {
                    Result.LayerAbsorbedDamage[i] = Reserve;
                    TotalDamageDealt += Reserve;
                    DamageToReserve -= Reserve;
                    RemainingDamage = DamageToReserve / ReserveDamageMods[i];
                    Reserve = 0.0f;
                    Result.ReserveDepleted[i] = true;
                }
                Result.NewReserves[i] = Reserve;

                OnReserveChanged.Broadcast(Layer.LayerTag, Reserve);
                if (Result.ReserveDepleted[i])
                    OnReserveDepleted.Broadcast(Layer.LayerTag);

                if (Result.LayerAbsorbedDamage[i] > 0.0f)
                    LastReserveDamageTimes[i] = World->GetTimeSeconds();
            }
        }
    }
    else
    {
        Result.LayerAbsorbedDamage.Empty();
        Result.NewReserves.Empty();
        Result.ReserveDepleted.Empty();
    }

    // 6. Пост-защитные эффекты (с прогнозируемой зоной и здоровьем)
    float FinalHealthDamage = RemainingDamage;
    if (Config && AttackInfo.OptionalEnemyEffects)
    {
        // Вычисляем прогнозируемое здоровье после применения урона
        float PredictedHealth = FMath::Max(0.0f, Health - FinalHealthDamage);
        FName PredictedZone = PredictHealthZone(PredictedHealth);

        FDamageProcessingContext Context;
        Context.IncomingDamage = RemainingDamage;
        Context.Config = Config;
        Context.Reserves = CurrentReserves;
        Context.ResistanceMultipliers = ResistanceMods;
        Context.ReserveDamageMultipliers = ReserveDamageMods;
        Context.IgnoreLayer = IgnoreLayer;
        Context.FinalHealthDamage = FinalHealthDamage;
        Context.StaggerChanceModifier = StaggerMod;
        Context.CurrentHealthZone = PredictedZone;          // зона после урона
        Context.PreviousHealthZone = OldHealthZone;        // зона до урона
        Context.Owner = GetOwner();
        Context.Weapon = AttackInfo.DamageSource;
        Context.ForceStaggerCooldown = RuntimeStaggerCooldown;
        Context.Health = PredictedHealth;                 // прогнозируемое здоровье

        FPostDefenseOutput Out = AttackInfo.OptionalEnemyEffects->PostDefenseProcessing(Context);
        FinalHealthDamage = Out.NewFinalHealthDamage;
        StaggerMod = Out.NewStaggerChanceModifier;
        bForceStagger = Out.bNewForceStagger;
        bNewForceStaggerCooldown = Out.bNewForceStaggerCooldown;
    }

    FinalHealthDamage *= FinalHealthMod;
    Result.FinalHealthDamage = FinalHealthDamage;

    // 7. Вычитание здоровья
    Health = FMath::Max(0.0f, Health - FinalHealthDamage);
    float HealthDelta = Health - OldHealth;
    Result.NewHealth = Health;

    TotalDamageDealt += FinalHealthDamage;
    Result.TotalDamageDealt = TotalDamageDealt;

    OnHealthChanged.Broadcast(Health, HealthDelta);

    if (FinalHealthDamage > 0.0f)
        LastHealthDamageTime = World->GetTimeSeconds();

    // 8. Определение zero-damage
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

    // 9. Стаггер (используем runtime-параметры)
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

        float Chance = GetStaggerChance(StaggerDamage) + StaggerMod;
        CurrentStaggerChance = Chance;
        if (bForceStagger || FMath::FRand() < Chance)
        {
            Result.bStaggerTriggered = true;
            bStaggerOnCooldown = true;
            OnStaggered.Broadcast();

            if (bForceStagger)
            {
                RuntimeStaggerCooldown = bNewForceStaggerCooldown;
                CurrentStaggerChance = 1.0f;
            }

            if (StaggerMod != 0.0f)
            {
                RuntimeStaggerCooldown = bNewForceStaggerCooldown;
            }

            World->GetTimerManager().SetTimer(StaggerCooldownTimer, this, &UEnemyDamageComponent::OnStaggerCooldownExpired, RuntimeStaggerCooldown, false);
        }
    }

    // 10. Обновление зоны здоровья (реальное состояние после вычитания)
    if (Config)
    {
        FName OldZone = CurrentHealthZoneTag;
        UpdateHealthZone();
        Result.CurrentHealthZoneTag = CurrentHealthZoneTag;
        Result.bHealthZoneChanged = (OldZone != CurrentHealthZoneTag);
        if (Result.bHealthZoneChanged)
            OnHealthZoneChanged.Broadcast(CurrentHealthZoneTag, OldZone);
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
        OnHealthDepleted.Broadcast(GetOwner(), Config ? Config->DeathBehaviorTag : NAME_None);
        if (World)
        {
            World->GetTimerManager().ClearTimer(HealthRegenTimer);
            World->GetTimerManager().ClearTimer(ReserveRegenTimer);
        }
    }

    // 12. Отладочный лог
    DebugLogDamage(Result, ModifiedDamage, HitZoneMult, DamageAfterZone, FinalHealthDamage, HealthDelta, bAnyReserveChanged);

    // 13. Финальное событие
    OnDamageTaken.Broadcast(Result);

    return Result;
}

void UEnemyDamageComponent::UpdateHealthZone()
{
    if (!Config || RuntimeHealthZones.Num() == 0)
    {
        CurrentHealthZoneTag = NAME_None;
        return;
    }

    for (const FHealthZoneDefinition& Zone : RuntimeHealthZones)
    {
        if (Health > Zone.LowerBound && Health <= Zone.UpperBound)
        {
            CurrentHealthZoneTag = Zone.ZoneTag;
            return;
        }
    }

    if (Health == 0.0f)
        return;

    UE_LOG(LogTemp, Error, TEXT("Health %f does not fit in any health zone! Config: %s. Falling back to last zone. (%s)"),
        Health, *GetNameSafe(Config), *GetNameSafe(GetOwner()));
    CurrentHealthZoneTag = RuntimeHealthZones.Last().ZoneTag;
}

void UEnemyDamageComponent::ApplyHealthRegen()
{
    if (!Config || bIsDead || !bHealthRegenEnabled) return;
    UWorld* World = GetWorld();
    if (!World) return;

    const FRegenerationParams& Params = RuntimeHealthRegenParams;
    if (Params.RegenRatePerSecond <= 0.0f) return;

    if (Params.bInterruptOnDamage && (World->GetTimeSeconds() - LastHealthDamageTime) < Params.RegenDelayAfterDamage)
        return;

    float Delta = Params.RegenRatePerSecond * 0.1f;
    float OldHealth = Health;
    Health = FMath::Min(MaxHealth, Health + Delta);
    float ActualDelta = Health - OldHealth;
    if (ActualDelta > 0.0f)
    {
        OnHealthChanged.Broadcast(Health, ActualDelta);
        FName OldZone = CurrentHealthZoneTag;
        UpdateHealthZone();
        if (OldZone != CurrentHealthZoneTag)
            OnHealthZoneChanged.Broadcast(CurrentHealthZoneTag, OldZone);

        if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
        {
            AActor* Owner = GetOwner();
            FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
            FString ZoneChange = (OldZone != CurrentHealthZoneTag) ? TEXT(" (zone changed)") : TEXT("");
            UE_LOG(LogTemp, Log, TEXT("[%s] Health regen: +%.1f (NewHealth %.1f) Zone: %s%s"),
                *OwnerName, ActualDelta, Health, *CurrentHealthZoneTag.ToString(), *ZoneChange);
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
        if (!bReserveRegenEnabled.IsValidIndex(i) || !bReserveRegenEnabled[i]) continue;

        const FRegenerationParams& Params = RuntimeReserveRegenParams[i];
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

            if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
            {
                AActor* Owner = GetOwner();
                FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
                UE_LOG(LogTemp, Log, TEXT("[%s] Reserve regen (%s): +%.1f (NewReserve %.1f)"),
                    *OwnerName, *Layer.LayerTag.ToString(), ActualDelta, Reserve);
            }
        }
    }
}

void UEnemyDamageComponent::OnStaggerCooldownExpired()
{
    bStaggerOnCooldown = false;
    OnStaggerCooldownEnded.Broadcast();

    AActor* Owner = GetOwner();
    FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
	UE_LOG(LogTemp, Log, TEXT("[%s] Stagger cooldown ended."), *OwnerName);
}

float UEnemyDamageComponent::GetStaggerChance(float DamageValue) const
{
    return FMath::Clamp(RuntimeBaseStaggerChance + DamageValue * RuntimeStaggerSusceptibility, 0.0f, 1.0f);
}

FName UEnemyDamageComponent::PredictHealthZone(float HealthValue) const
{
    if (!Config || RuntimeHealthZones.Num() == 0)
        return NAME_None;

    for (const FHealthZoneDefinition& Zone : RuntimeHealthZones)
    {
        if (HealthValue > Zone.LowerBound && HealthValue <= Zone.UpperBound)
            return Zone.ZoneTag;
    }

    // Если здоровье не попало ни в одну зону (например, из-за ошибки) – берём последнюю (обычно 0-я)
    return RuntimeHealthZones.Last().ZoneTag;
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

// ---- Здоровье ----
void UEnemyDamageComponent::Heal(float Amount, bool bInstant)
{
    if (bIsDead) return;
    if (Amount <= 0.0f) return;

    if (bInstant)
    {
        float OldHealth = Health;
        Health = FMath::Min(Health + Amount, MaxHealth);
        float Delta = Health - OldHealth;
        if (Delta > 0.0f)
        {
            OnHealthChanged.Broadcast(Health, Delta);
            // Обновляем зону здоровья
            FName OldZone = CurrentHealthZoneTag;
            UpdateHealthZone();
            if (OldZone != CurrentHealthZoneTag)
                OnHealthZoneChanged.Broadcast(CurrentHealthZoneTag, OldZone);
        }
    }
    else
    {
        // Постепенное восстановление – можно запустить регенерацию с заданной скоростью,
        // но проще воспользоваться существующей регенерацией, изменив её параметры.
        // Для простоты оставим мгновенное, а постепенное реализуем через изменение параметров регенерации.
        // Либо можно запустить отдельный таймер, но это усложнит.
        // Предлагаем сделать только мгновенное восстановление, а для постепенного использовать настройку регенерации.
        // Поэтому просто игнорируем bInstant == false или делаем то же самое.
        Heal(Amount, true);
    }
}

void UEnemyDamageComponent::HealToMax(bool bInstant)
{
    if (bIsDead) return;
    float Missing = MaxHealth - Health;
    if (Missing > 0.0f)
        Heal(Missing, bInstant);
}

void UEnemyDamageComponent::SetHealth(float NewHealth)
{
    if (bIsDead) return;

    float ClampedHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
    if (FMath::IsNearlyEqual(ClampedHealth, Health)) return;

    if (ClampedHealth <= 0.0f && !bIsDead)
    {
        //bIsDead = true;
        Kill();
        //OnHealthDepleted.Broadcast(GetOwner(), Config ? Config->DeathBehaviorTag : NAME_None);
        UWorld* World = GetWorld();
        if (World)
        {
            World->GetTimerManager().ClearTimer(HealthRegenTimer);
            World->GetTimerManager().ClearTimer(ReserveRegenTimer);
        }
    }

    float OldHealth = Health;
    Health = ClampedHealth;
    float Delta = Health - OldHealth;

    OnHealthChanged.Broadcast(Health, Delta);

    FName OldZone = CurrentHealthZoneTag;
    UpdateHealthZone();
    if (OldZone != CurrentHealthZoneTag)
        OnHealthZoneChanged.Broadcast(CurrentHealthZoneTag, OldZone);


}

void UEnemyDamageComponent::SetMaxHealth(float NewMaxHealth)
{
    if (NewMaxHealth <= 0.0f) return;

    float OldMaxHealth = MaxHealth;
    MaxHealth = NewMaxHealth;

    // Масштабируем зоны пропорционально
    if (RuntimeHealthZones.Num() > 0 && OldMaxHealth > 0.0f)
    {
        float Scale = NewMaxHealth / OldMaxHealth;
        for (FHealthZoneDefinition& Zone : RuntimeHealthZones)
        {
            Zone.UpperBound *= Scale;
            Zone.LowerBound *= Scale;
        }
        CheckHealthZone(); // валидация
    }

    // Корректируем здоровье, если оно превышает новый максимум
    if (Health > MaxHealth)
    {
        Health = MaxHealth;
        OnHealthChanged.Broadcast(Health, 0.0f);
    }

    // Сохраняем старую зону, обновляем, генерируем событие при изменении
    FName OldZone = CurrentHealthZoneTag;
    UpdateHealthZone();
    if (OldZone != CurrentHealthZoneTag)
    {
        OnHealthZoneChanged.Broadcast(CurrentHealthZoneTag, OldZone);

        // Логирование
        if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
        {
            AActor* Owner = GetOwner();
            FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
            UE_LOG(LogTemp, Log, TEXT("[%s] Health zone changed from %s to %s due to MaxHealth change (NewMax=%.1f)"),
                *OwnerName, *OldZone.ToString(), *CurrentHealthZoneTag.ToString(), MaxHealth);
        }
    }
    /*
    else
    {
        // Опционально: лог, что зона не изменилась (если нужно)
        if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
        {
            AActor* Owner = GetOwner();
            FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
            UE_LOG(LogTemp, Log, TEXT("[%s] Health zone unchanged after MaxHealth change (Zone: %s, NewMax=%.1f)"),
                *OwnerName, *CurrentHealthZoneTag.ToString(), MaxHealth);
        }
    }
    */
}

// ---- Резервы ----
void UEnemyDamageComponent::RestoreReserve(int32 LayerIndex, float Amount, bool bInstant)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex)) return;
    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Reserve) return;
    if (bIsDead) return;

    float& Reserve = CurrentReserves[LayerIndex];
    float MaxRes = Config->DefenseLayers[LayerIndex].MaxReserve;
    float OldReserve = Reserve;
    Reserve = FMath::Min(Reserve + Amount, MaxRes);
    float Delta = Reserve - OldReserve;
    if (Delta > 0.0f)
    {
        OnReserveChanged.Broadcast(Config->DefenseLayers[LayerIndex].LayerTag, Reserve);
    }
}

void UEnemyDamageComponent::RestoreReserveToMax(int32 LayerIndex, bool bInstant)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex)) return;
    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Reserve) return;
    float MaxRes = Config->DefenseLayers[LayerIndex].MaxReserve;
    float Current = CurrentReserves[LayerIndex];
    if (Current < MaxRes)
        RestoreReserve(LayerIndex, MaxRes - Current, bInstant);
}

void UEnemyDamageComponent::SetReserve(int32 LayerIndex, float NewValue)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex)) return;
    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Reserve) return;
    if (bIsDead) return;

    float MaxRes = Config->DefenseLayers[LayerIndex].MaxReserve;
    float Clamped = FMath::Clamp(NewValue, 0.0f, MaxRes);
    if (FMath::IsNearlyEqual(Clamped, CurrentReserves[LayerIndex])) return;

    CurrentReserves[LayerIndex] = Clamped;
    OnReserveChanged.Broadcast(Config->DefenseLayers[LayerIndex].LayerTag, Clamped);
}

// ---- Сопротивление ----
void UEnemyDamageComponent::SetResistanceMultiplier(int32 LayerIndex, float NewMultiplier)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex)) return;
    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Resistance) return;
    if (!RuntimeResistanceMultipliers.IsValidIndex(LayerIndex)) return;

    RuntimeResistanceMultipliers[LayerIndex] = FMath::Max(0.0f, NewMultiplier);
}

void UEnemyDamageComponent::SetResistanceMultiplierByTag(FName LayerTag, float NewMultiplier)
{
    if (!Config) return;
    for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
    {
        if (Config->DefenseLayers[i].LayerTag == LayerTag && Config->DefenseLayers[i].LayerType == EDefenseLayerType::Resistance)
        {
            SetResistanceMultiplier(i, NewMultiplier);
            return;
        }
    }
}

// ---- Регенерация здоровья ----
void UEnemyDamageComponent::SetHealthRegenEnabled(bool bEnabled)
{
    bHealthRegenEnabled = bEnabled;
    if (!bEnabled)
    {
        UWorld* World = GetWorld();
        if (World)
            World->GetTimerManager().ClearTimer(HealthRegenTimer);
    }
    else
    {
        // Перезапустить таймер, если он не работает
        UWorld* World = GetWorld();
        if (World && Config && RuntimeHealthRegenParams.RegenRatePerSecond > 0.0f)
        {
            World->GetTimerManager().SetTimer(HealthRegenTimer, this, &UEnemyDamageComponent::ApplyHealthRegen, 0.1f, true);
        }
    }
}

void UEnemyDamageComponent::SetHealthRegenRate(float Rate)
{
    RuntimeHealthRegenParams.RegenRatePerSecond = FMath::Max(0.0f, Rate);
    // Если регенерация включена и таймер не работает – перезапустить
    if (bHealthRegenEnabled && Rate > 0.0f)
    {
        UWorld* World = GetWorld();
        if (World && !World->GetTimerManager().IsTimerActive(HealthRegenTimer))
        {
            World->GetTimerManager().SetTimer(HealthRegenTimer, this, &UEnemyDamageComponent::ApplyHealthRegen, 0.1f, true);
        }
    }
}

void UEnemyDamageComponent::SetHealthRegenDelay(float Delay)
{
    RuntimeHealthRegenParams.RegenDelayAfterDamage = FMath::Max(0.0f, Delay);
}

void UEnemyDamageComponent::SetHealthRegenInterruptOnDamage(bool bInterrupt)
{
    RuntimeHealthRegenParams.bInterruptOnDamage = bInterrupt;
}

void UEnemyDamageComponent::SetHealthZones(const TArray<FHealthZoneDefinition>& NewZones)
{
    RuntimeHealthZones = NewZones;
    // Сортируем по убыванию UpperBound (как в конфиге)
    RuntimeHealthZones.Sort([](const FHealthZoneDefinition& A, const FHealthZoneDefinition& B) {
        return A.UpperBound > B.UpperBound;
        });
    CheckHealthZone();
    UpdateHealthZone();
}

// ---- Регенерация резервов ----
void UEnemyDamageComponent::SetReserveRegenEnabled(int32 LayerIndex, bool bEnabled)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex)) return;
    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Reserve) return;
    if (!bReserveRegenEnabled.IsValidIndex(LayerIndex)) return;

    bReserveRegenEnabled[LayerIndex] = bEnabled;
    // Здесь можно перезапустить таймер, если он общий – но у нас общий таймер на все резервы.
    // Поэтому проще проверять флаг внутри ApplyReserveRegen.
}

void UEnemyDamageComponent::SetReserveRegenRate(int32 LayerIndex, float Rate)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex)) return;
    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Reserve) return;
    if (!RuntimeReserveRegenParams.IsValidIndex(LayerIndex)) return;

    RuntimeReserveRegenParams[LayerIndex].RegenRatePerSecond = FMath::Max(0.0f, Rate);
}

void UEnemyDamageComponent::SetReserveRegenDelay(int32 LayerIndex, float Delay)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex)) return;
    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Reserve) return;
    if (!RuntimeReserveRegenParams.IsValidIndex(LayerIndex)) return;

    RuntimeReserveRegenParams[LayerIndex].RegenDelayAfterDamage = FMath::Max(0.0f, Delay);
}

void UEnemyDamageComponent::SetReserveRegenInterruptOnDamage(int32 LayerIndex, bool bInterrupt)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex)) return;
    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Reserve) return;
    if (!RuntimeReserveRegenParams.IsValidIndex(LayerIndex)) return;

    RuntimeReserveRegenParams[LayerIndex].bInterruptOnDamage = bInterrupt;
}

// ---- Стаггер ----
void UEnemyDamageComponent::SetStaggerParams(float BaseChance, float Susceptibility, float Cooldown)
{
    RuntimeBaseStaggerChance = FMath::Clamp(BaseChance, 0.0f, 1.0f);
    RuntimeStaggerSusceptibility = FMath::Max(0.0f, Susceptibility);
    RuntimeStaggerCooldown = FMath::Max(0.0f, Cooldown);
}

// ---- Принудительная смерть ----
void UEnemyDamageComponent::Kill()
{
    if (bIsDead) return;
    if (Health <= 0.0f) return; // уже мёртв

    Health = 0.0f;
    bIsDead = true;
    OnHealthChanged.Broadcast(0, -Health); // дельта = -старое здоровье
    Health = 0.0f;
    FName OldZone = CurrentHealthZoneTag;
    UpdateHealthZone();
    if (OldZone != CurrentHealthZoneTag)
        OnHealthZoneChanged.Broadcast(CurrentHealthZoneTag, OldZone);

    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(HealthRegenTimer);
        World->GetTimerManager().ClearTimer(ReserveRegenTimer);
    }

    // Отладочный лог (если включён)
    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[%s] Killed by script."), *OwnerName);
    }

    OnHealthDepleted.Broadcast(GetOwner(), Config ? Config->DeathBehaviorTag : NAME_None);
}

void UEnemyDamageComponent::DebugLogDamage(const FDamageResult& Result, float ModifiedDamage, float HitZoneMult, float DamageAfterZone, float FinalHealthDamage, float HealthDelta, bool bAnyReserveChanged)
{
    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() == 0)
        return;

    AActor* Owner = GetOwner();
    FString OwnerName = Owner ? Owner->GetName() : TEXT("None");

    FString LogString;
    LogString += FString::Printf(TEXT("[%s] Incoming %.1f"), *OwnerName, ModifiedDamage);

    //if (HitZoneMult != 1.0f)
        LogString += CurrentHitZoneName.IsNone() ? FString::Printf(TEXT(" -> HitRegion x%.2f (%.1f)"), HitZoneMult, DamageAfterZone) : FString::Printf(TEXT(" -> HitRegion [%s] x%.2f (%.1f)"), *CurrentHitZoneName.ToString(), HitZoneMult, DamageAfterZone);
    //else
    //    LogString += FString::Printf(TEXT(" -> HitRegion x1.00 (%.1f)"), DamageAfterZone);

    if (Config && Config->DefenseLayers.Num() > 0)
    {
        for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
        {
            const FDefenseLayer& Layer = Config->DefenseLayers[i];
            if (Layer.LayerType == EDefenseLayerType::Resistance)
            {
                float absorbed = Result.LayerAbsorbedDamage[i];
                if (absorbed > 0.0f)
                    LogString += FString::Printf(TEXT(" -> Resistance [Tag: %s] x%.2f (absorbed %.1f)"), *Layer.LayerTag.ToString(), Layer.ResistanceMultiplier, absorbed);
                else
                    LogString += FString::Printf(TEXT(" -> Resistance [Tag: %s] x%.2f"), *Layer.LayerTag.ToString(), Layer.ResistanceMultiplier);
            }
            else if (Layer.LayerType == EDefenseLayerType::Reserve)
            {
                float absorbed = Result.LayerAbsorbedDamage[i];
                if (absorbed > 0.0f)
                    LogString += FString::Printf(TEXT(" -> Reserve [Tag: %s] -%.1f (remaining %.1f)"), *Layer.LayerTag.ToString(), absorbed, CurrentReserves[i]);
                else
                    LogString += FString::Printf(TEXT(" -> Reserve [Tag: %s] no loss (%.1f)"), *Layer.LayerTag.ToString(), CurrentReserves[i]);
            }
        }
    }
    else
    {
        LogString += TEXT(" -> No defense layers");
    }

    LogString += FString::Printf(TEXT(" -> Health -%.1f (NewHealth %.1f)"), FinalHealthDamage, Health);

    if (Config && Config->HealthZones.Num() > 0)
    {
        LogString += FString::Printf(TEXT(" -> Zone: %s"), *CurrentHealthZoneTag.ToString());
        if (Result.bHealthZoneChanged)
            LogString += TEXT(" (changed)");
    }
    else
    {
        LogString += TEXT(" -> Zone: none");
    }

    LogString += Result.bStaggerTriggered ? FString::Printf(TEXT(" -> Stagger: YES (%f)"), CurrentStaggerChance) : FString::Printf(TEXT(" -> Stagger: no (%f)"), CurrentStaggerChance);

    TArray<FString> Signals;

    Signals.Add(TEXT("OnDamageTaken"));

    if (FinalHealthDamage > 0.0f || !FMath::IsNearlyEqual(HealthDelta, 0.0f))
        Signals.Add(TEXT("OnHealthChanged"));
    if (bAnyReserveChanged)
        Signals.Add(TEXT("OnReserveChanged"));
    if (Result.ReserveDepleted.Contains(true))
        Signals.Add(TEXT("OnReserveDepleted"));
    if (Result.bStaggerTriggered)
        Signals.Add(TEXT("OnStaggered"));
    if (Result.bHealthZoneChanged)
        Signals.Add(TEXT("OnHealthZoneChanged"));
    if (Result.bKilled)
        Signals.Add(TEXT("OnHealthDepleted"));

    if (Signals.Num() > 0)
        LogString += TEXT(" -> Signals: ") + FString::Join(Signals, TEXT(", "));
    else
        LogString += TEXT(" -> Signals: none");

    UE_LOG(LogTemp, Log, TEXT("%s"), *LogString);
}