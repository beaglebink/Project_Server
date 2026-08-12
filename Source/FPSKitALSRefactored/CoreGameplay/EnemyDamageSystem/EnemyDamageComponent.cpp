// EnemyDamageComponent.cpp
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
    LastHealthDamageTime = World->GetTimeSeconds();//0.0f;

    if (World)
    {
        World->GetTimerManager().ClearTimer(HealthRegenTimer);
        World->GetTimerManager().ClearTimer(ReserveRegenTimer);
        World->GetTimerManager().ClearTimer(StaggerCooldownTimer);
    }

    if (Config)
    {
        Health = FMath::Min(InitialHealth, Config->MaxHealth);
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

        Config->HealthZones.Sort([](const FHealthZoneDefinition& A, const FHealthZoneDefinition& B) {
            return A.UpperBound > B.UpperBound;
            });
        UpdateHealthZone();

        if (Config->HealthZones.Num() > 0)
        {
            if (!FMath::IsNearlyEqual(Config->HealthZones[0].UpperBound, Config->MaxHealth, 0.001f))
            {
                UE_LOG(LogTemp, Error, TEXT("Health zone invalid: first zone UpperBound (%f) != MaxHealth (%f) in config %s"),
                    Config->HealthZones[0].UpperBound, Config->MaxHealth, *GetNameSafe(Config));
            }
            if (!FMath::IsNearlyEqual(Config->HealthZones.Last().LowerBound, 0.0f, 0.001f))
            {
                UE_LOG(LogTemp, Error, TEXT("Health zone invalid: last zone LowerBound (%f) != 0 in config %s"),
                    Config->HealthZones.Last().LowerBound, *GetNameSafe(Config));
            }
            for (int32 i = 0; i < Config->HealthZones.Num() - 1; ++i)
            {
                if (!FMath::IsNearlyEqual(Config->HealthZones[i].LowerBound, Config->HealthZones[i + 1].UpperBound, 0.001f))
                {
                    UE_LOG(LogTemp, Error, TEXT("Health zone gap/overlap between %s and %s in config %s"),
                        *Config->HealthZones[i].ZoneTag.ToString(), *Config->HealthZones[i + 1].ZoneTag.ToString(), *GetNameSafe(Config));
                }
            }
        }

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
        Health = FMath::Max(0.0f, InitialHealth);
        CurrentReserves.Empty();
        LastReserveDamageTimes.Empty();
        CurrentHealthZoneTag = NAME_None;
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

    if (ModifiedDamage < 0)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");

        FString LogString;
        LogString += FString::Printf(TEXT("[%s] Error, damage is less than zero."), *OwnerName);

        UE_LOG(LogTemp, Log, TEXT("%s"), *LogString);
        return Result;
    }

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

    // 4. Подготовка контекста и модификаторов
    float RemainingDamage = DamageAfterZone;
    TArray<float> ResistanceMods;
    TArray<float> ReserveDamageMods;
    TArray<bool> BypassReserve;
    TArray<bool> IgnoreLayer;
    float FinalHealthMod = 1.0f;
    float StaggerMod = 0.0f;
    bool bForceStagger = false;

    if (Config)
    {
        int32 NumLayers = Config->DefenseLayers.Num();
        ResistanceMods.Init(1.0f, NumLayers);
        ReserveDamageMods.Init(1.0f, NumLayers);
        BypassReserve.Init(false, NumLayers);
        IgnoreLayer.Init(false, NumLayers);

        // Формируем контекст
        FDamageProcessingContext Context;
        Context.IncomingDamage = RemainingDamage;
        Context.Config = Config;
        Context.Reserves = CurrentReserves;
        Context.ResistanceMultipliers = ResistanceMods;
        Context.ReserveDamageMultipliers = ReserveDamageMods;
        Context.BypassReserve = BypassReserve;
        Context.IgnoreLayer = IgnoreLayer;
        Context.FinalHealthDamage = 0.0f; // пока не известно
        Context.StaggerChanceModifier = 0.0f;
        Context.bForceStagger = false;

        // 1. ModifyDamageProcessing
        if (AttackInfo.OptionalEnemyEffects)
        {
            FPreDefenseOutput Out = AttackInfo.OptionalEnemyEffects->ModifyDamageProcessing(Context);
            RemainingDamage = Out.NewIncomingDamage;
            // Обновляем контекст, чтобы дальнейшие этапы видели новый урон
            Context.IncomingDamage = RemainingDamage;
        }

        // 2. ApplyDefenseModifiers
        if (AttackInfo.OptionalEnemyEffects)
        {
            FDefenseOutput Out = AttackInfo.OptionalEnemyEffects->ApplyDefenseModifiers(Context);
            ResistanceMods = Out.NewModifiers.ResistanceMultipliers;
            ReserveDamageMods = Out.NewModifiers.ReserveDamageMultipliers;
            BypassReserve = Out.NewModifiers.BypassReserve;
            IgnoreLayer = Out.NewModifiers.IgnoreLayer;
            // Обновляем контекст для последующих этапов (хотя они не используют эти массивы, но для полноты)
            Context.ResistanceMultipliers = ResistanceMods;
            Context.ReserveDamageMultipliers = ReserveDamageMods;
            Context.BypassReserve = BypassReserve;
            Context.IgnoreLayer = IgnoreLayer;
        }
    }

    // 5. Применение слоёв защиты с учётом модификаторов
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

            // Пропуск слоя, если игнорируется
            if (IgnoreLayer[i])
                continue;

            const FDefenseLayer& Layer = Config->DefenseLayers[i];
            if (Layer.LayerType == EDefenseLayerType::Resistance)
            {
                float EffectiveResistance = FMath::Clamp(Layer.ResistanceMultiplier * ResistanceMods[i], 0.0f, 1.0f);
                float Absorbed = RemainingDamage * (1.0f - EffectiveResistance);
                Result.LayerAbsorbedDamage[i] = Absorbed;
                RemainingDamage *= EffectiveResistance;
            }
            else if (Layer.LayerType == EDefenseLayerType::Reserve)
            {
                // Если резерв игнорируется (Bypass), урон не списывается
                if (BypassReserve[i])
                {
                    continue;
                }

                float& Reserve = CurrentReserves[i];
                // Урон по резерву модифицируется
                float DamageToReserve = RemainingDamage * ReserveDamageMods[i];
                if (DamageToReserve <= 0.0f)
                    continue;

                if (DamageToReserve <= Reserve)
                {
                    Result.LayerAbsorbedDamage[i] = DamageToReserve;
                    Reserve -= DamageToReserve;
                    TotalDamageDealt += DamageToReserve;
                    RemainingDamage = 0.0f; // урон полностью поглощён резервом
                }
                else
                {
                    Result.LayerAbsorbedDamage[i] = Reserve;
                    TotalDamageDealt += Reserve;
                    DamageToReserve -= Reserve;
                    RemainingDamage = DamageToReserve / ReserveDamageMods[i]; // остаток урона пересчитываем без модификатора
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

    // 6. Пост-защитные эффекты
    float FinalHealthDamage = RemainingDamage;
    if (Config && AttackInfo.OptionalEnemyEffects)
    {
        // Обновляем контекст для финального этапа
        FDamageProcessingContext Context;
        Context.IncomingDamage = RemainingDamage; // уже не актуально
        Context.Config = Config;
        Context.Reserves = CurrentReserves;
        Context.ResistanceMultipliers = ResistanceMods;
        Context.ReserveDamageMultipliers = ReserveDamageMods;
        Context.BypassReserve = BypassReserve;
        Context.IgnoreLayer = IgnoreLayer;
        Context.FinalHealthDamage = FinalHealthDamage;
        Context.StaggerChanceModifier = StaggerMod;
        Context.bForceStagger = bForceStagger;

        FPostDefenseOutput Out = AttackInfo.OptionalEnemyEffects->PostDefenseProcessing(Context);
        FinalHealthDamage = Out.NewFinalHealthDamage;
        StaggerMod = Out.NewStaggerChanceModifier;
        bForceStagger = Out.bNewForceStagger;
    }

    // 7. Вычитание здоровья
    Health = FMath::Max(0.0f, Health - FinalHealthDamage);
    float HealthDelta = Health - OldHealth;
    Result.NewHealth = Health;

    // Добавляем урон по здоровью в общий реальный урон
    TotalDamageDealt += FinalHealthDamage;
    Result.TotalDamageDealt = TotalDamageDealt;

    // Всегда генерируем событие изменения здоровья (включая нулевое изменение)
    if(HealthDelta > 0.0f)
        OnHealthChanged.Broadcast(Health, HealthDelta);

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

        float Chance = GetStaggerChance(StaggerDamage) + StaggerMod;
        if (bForceStagger || FMath::FRand() < Chance)
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
        OnHealthDepleted.Broadcast(GetOwner(), Config->DeathBehaviorTag);
        // Останавливаем регенерацию
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

    UE_LOG(LogTemp, Error, TEXT("Health %f does not fit in any health zone! Config: %s. Falling back to last zone."),
        Health, *GetNameSafe(Config));
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

void UEnemyDamageComponent::DebugLogDamage(const FDamageResult& Result, float ModifiedDamage, float HitZoneMult, float DamageAfterZone, float FinalHealthDamage, float HealthDelta, bool bAnyReserveChanged)
{
    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() == 0)
        return;

    AActor* Owner = GetOwner();
    FString OwnerName = Owner ? Owner->GetName() : TEXT("None");

    FString LogString;
    LogString += FString::Printf(TEXT("[%s] Incoming %.1f"), *OwnerName, ModifiedDamage);

    if (HitZoneMult != 1.0f)
        LogString += FString::Printf(TEXT(" -> HitRegion x%.2f (%.1f)"), HitZoneMult, DamageAfterZone);
    else
        LogString += FString::Printf(TEXT(" -> HitRegion x1.00 (%.1f)"), DamageAfterZone);

    if (Config && Config->DefenseLayers.Num() > 0)
    {
        for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
        {
            const FDefenseLayer& Layer = Config->DefenseLayers[i];
            if (Layer.LayerType == EDefenseLayerType::Resistance)
            {
                float absorbed = Result.LayerAbsorbedDamage[i];
                if (absorbed > 0.0f)
                    LogString += FString::Printf(TEXT(" -> Resistance x%.2f (absorbed %.1f)"), Layer.ResistanceMultiplier, absorbed);
                else
                    LogString += FString::Printf(TEXT(" -> Resistance x%.2f"), Layer.ResistanceMultiplier);
            }
            else if (Layer.LayerType == EDefenseLayerType::Reserve)
            {
                float absorbed = Result.LayerAbsorbedDamage[i];
                if (absorbed > 0.0f)
                    LogString += FString::Printf(TEXT(" -> Reserve -%.1f (remaining %.1f)"), absorbed, CurrentReserves[i]);
                else
                    LogString += FString::Printf(TEXT(" -> Reserve no loss (%.1f)"), CurrentReserves[i]);
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

    LogString += Result.bStaggerTriggered ? TEXT(" -> Stagger: YES") : TEXT(" -> Stagger: no");

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