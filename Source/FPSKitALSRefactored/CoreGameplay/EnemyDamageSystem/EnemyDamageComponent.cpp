#include "EnemyDamageComponent.h"
#include "EnemyDamageConfig.h"
#include "DamageProcessingEffect.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "JsonObjectConverter.h"
#include "Json.h"

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

FEnemyDamageSaveData UEnemyDamageComponent::SaveState() const
{
    FEnemyDamageSaveData Data;
    UClass* Class = GetClass();
    UStruct* SaveStruct = FEnemyDamageSaveData::StaticStruct();

    for (TFieldIterator<FProperty> It(Class); It; ++It)
    {
        FProperty* Prop = *It;
        if (Prop->HasAnyPropertyFlags(CPF_SaveGame))
        {
            FProperty* StructProp = SaveStruct->FindPropertyByName(Prop->GetFName());
            if (!StructProp)
                continue;

            // Check type compatibility (we can use Prop->SameType or a softer check)
            // Проверяем совместимость типов (можно использовать Prop->SameType или более мягкую проверку)
            if (Prop->SameType(StructProp))
            {
                const void* SrcAddr = Prop->ContainerPtrToValuePtr<const void>(this);
                void* DstAddr = StructProp->ContainerPtrToValuePtr<void>(&Data);
                Prop->CopyCompleteValue(DstAddr, SrcAddr);
            }
        }
    }
    return Data;
}

void UEnemyDamageComponent::LoadState(const FEnemyDamageSaveData& SaveData)
{
    UClass* Class = GetClass();
    UStruct* SaveStruct = FEnemyDamageSaveData::StaticStruct();

    for (TFieldIterator<FProperty> It(Class); It; ++It)
    {
        FProperty* Prop = *It;
        if (Prop->HasAnyPropertyFlags(CPF_SaveGame))
        {
            FProperty* StructProp = SaveStruct->FindPropertyByName(Prop->GetFName());
            if (!StructProp)
                continue;

            if (Prop->SameType(StructProp))
            {
                const void* SrcAddr = StructProp->ContainerPtrToValuePtr<const void>(&SaveData);
                void* DstAddr = Prop->ContainerPtrToValuePtr<void>(this);
                Prop->CopyCompleteValue(DstAddr, SrcAddr);
            }
        }
    }

    // After loading data, update the state (timers, zones, etc.)
    // После загрузки данных обновляем состояние (таймеры, зоны и т.д.)
    UpdateHealthZone();

    if (Health <= 0.0f && !bIsDead)
    {
        Kill();
        return;
    }

    if (bIsDead)
    {
        UWorld* World = GetWorld();
        if (World)
        {
            World->GetTimerManager().ClearTimer(HealthRegenTimer);
            World->GetTimerManager().ClearTimer(ReserveRegenTimer);
        }
        return;
    }

    UWorld* World = GetWorld();
    if (!World) return;

    // 1. Health
    // 1. Здоровье
    World->GetTimerManager().ClearTimer(HealthRegenTimer);
    if (bHealthRegenEnabled && RuntimeHealthRegenParams.RegenRatePerSecond > 0.0f)
    {
        World->GetTimerManager().SetTimer(HealthRegenTimer, this, &UEnemyDamageComponent::ApplyHealthRegen, 0.1f, true);
    }

    // 2. Reserves
    // 2. Резервы
    World->GetTimerManager().ClearTimer(ReserveRegenTimer);
    bool bAnyReserveRegen = false;
    if (Config)
    {
        for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
        {
            if (Config->DefenseLayers[i].LayerType == EDefenseLayerType::Reserve &&
                bReserveRegenEnabled.IsValidIndex(i) && bReserveRegenEnabled[i] &&
                RuntimeReserveRegenParams.IsValidIndex(i) && RuntimeReserveRegenParams[i].RegenRatePerSecond > 0.0f)
            {
                bAnyReserveRegen = true;
                break;
            }
        }
    }
    if (bAnyReserveRegen)
    {
        World->GetTimerManager().SetTimer(ReserveRegenTimer, this, &UEnemyDamageComponent::ApplyReserveRegen, 0.1f, true);
    }

    // 3. Stagger
    // 3. Стаггер
    World->GetTimerManager().ClearTimer(StaggerCooldownTimer);
    if (bStaggerOnCooldown && RuntimeStaggerCooldown > 0.0f)
    {
        World->GetTimerManager().SetTimer(StaggerCooldownTimer, this, &UEnemyDamageComponent::OnStaggerCooldownExpired, RuntimeStaggerCooldown, false);
    }
    else if (bStaggerOnCooldown && RuntimeStaggerCooldown <= 0.0f)
    {
        bStaggerOnCooldown = false;
    }
}

// ---- Serialization to JSON string ----
// ---- Сериализация в JSON-строку ----
FString UEnemyDamageComponent::SerializeToString() const
{
    FEnemyDamageSaveData Data = SaveState();
    FString JsonString;
    if (!FJsonObjectConverter::UStructToJsonObjectString(Data, JsonString))
    {
        UE_LOG(LogTemp, Error, TEXT("SerializeToString: Failed to convert save data to JSON."));
        return TEXT("");
    }
    return JsonString;
}

// ---- Deserialization from JSON string ----
// ---- Десериализация из JSON-строки ----
bool UEnemyDamageComponent::DeserializeFromString(const FString& Data)
{
    if (Data.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("DeserializeFromString: Empty data string."));
        return false;
    }

    FEnemyDamageSaveData SaveData;
    if (!FJsonObjectConverter::JsonObjectStringToUStruct(Data, &SaveData))
    {
        UE_LOG(LogTemp, Error, TEXT("DeserializeFromString: Failed to parse JSON data: %s"), *Data);
        return false;
    }

    LoadState(SaveData);
    return true;
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

    // Reset runtime fields
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

        // Initialize reserves and runtime parameters
        // Инициализация резервов и runtime-параметров
        for (const FDefenseLayer& Layer : Config->DefenseLayers)
        {
            if (Layer.LayerType == EDefenseLayerType::Reserve)
            {
                CurrentReserves.Add(FMath::Min(Layer.InitialReserve, Layer.MaxReserve));
                RuntimeResistanceMultipliers.Add(1.0f);
                RuntimeReserveRegenParams.Add(Layer.ReserveRegen);
                bReserveRegenEnabled.Add(true);
                RuntimeMaxReserves.Add(Layer.MaxReserve);
            }
            else // Resistance
            {
                CurrentReserves.Add(0.0f);
                RuntimeResistanceMultipliers.Add(Layer.ResistanceMultiplier);
                RuntimeReserveRegenParams.Add(FRegenerationParams());
                bReserveRegenEnabled.Add(false);
                RuntimeMaxReserves.Add(0.0f);
            }
            LastReserveDamageTimes.Add(0.0f);
        }

        // Sort and validate health zones
        // Сортировка и проверка зон здоровья
        Config->HealthZones.Sort([](const FHealthZoneDefinition& A, const FHealthZoneDefinition& B) {
            return A.UpperBound > B.UpperBound;
            });

        RuntimeHealthZones = Config->HealthZones;
        UpdateHealthZone();
        CheckHealthZone();

        // Copy health regeneration and stagger parameters
        // Копируем параметры регенерации здоровья и стаггера
        RuntimeHealthRegenParams = Config->HealthRegen;
        RuntimeBaseStaggerChance = Config->BaseStaggerChance;
        RuntimeStaggerSusceptibility = Config->StaggerSusceptibility;
        RuntimeStaggerCooldown = Config->StaggerCooldown;
        RuntimeStaggerInputType = Config->StaggerInput;

        // Start regeneration timers
        // Запуск таймеров регенерации
        if (World)
        {
            if (bHealthRegenEnabled && RuntimeHealthRegenParams.RegenRatePerSecond > 0.0f)
            {
                World->GetTimerManager().SetTimer(HealthRegenTimer, this, &UEnemyDamageComponent::ApplyHealthRegen, 0.1f, true);
            }
            // Reserve regeneration – at least one enabled reserve with positive rate
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
    else // Without config – only health
    // Без конфига – только здоровье
    {
        Health = FMath::Max(0.0f, InitialHealth);
        MaxHealth = Health;
        CurrentReserves.Empty();
        LastReserveDamageTimes.Empty();
        CurrentHealthZoneTag = NAME_None;
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

    // 1. Check for duplicate attack
    // 1. Проверка дубликата атаки
    if (RecentAttackIDs.Contains(AttackInfo.AttackID))
        return Result;
    RecentAttackIDs.Add(AttackInfo.AttackID);
    if (RecentAttackIDs.Num() > MaxRecentAttacks)
        RecentAttackIDs.RemoveAt(0);

    CheckHealthZone(); // validate zones (in case config changed) // валидация зон (на случай изменения конфига)

    // Save state before processing
    // Сохраняем состояние до обработки
    float OldHealth = Health;
    TArray<float> OldReserves = CurrentReserves;
    FName OldHealthZone = CurrentHealthZoneTag;

    if (AttackInfo.BaseDamage < 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Negative damage"));
        return Result;
    }

    // 2. Calculate modified damage
    // 2. Расчёт модифицированного урона
    float ModifiedDamage = AttackInfo.BaseDamage;
    Result.IncomingDamage = ModifiedDamage;

    // 3. Hit zone (bones + components)
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

    // 4. Effect context (Modify + ApplyDefense)
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
        Context.Instigator = AttackInfo.Instigator;
        Context.DamageSource = AttackInfo.DamageSource;
        Context.ForceStaggerCooldown = RuntimeStaggerCooldown;
        Context.Health = Health; // current health before damage // текущее здоровье до урона
        Context.IsStagger = bStaggerOnCooldown;

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

    // 5. Apply defense layers (taking runtime resistances into account)
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

    // 6. Post-defense effects (with predicted zone and health)
    // 6. Пост-защитные эффекты (с прогнозируемой зоной и здоровьем)
    float FinalHealthDamage = RemainingDamage;
    if (Config && AttackInfo.OptionalEnemyEffects)
    {
        // Calculate predicted health after applying damage
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
        Context.CurrentHealthZone = PredictedZone;          // zone after damage // зона после урона
        Context.PreviousHealthZone = OldHealthZone;        // zone before damage // зона до урона
        Context.Owner = GetOwner();
        Context.Instigator = AttackInfo.Instigator;
        Context.DamageSource = AttackInfo.DamageSource;
        Context.ForceStaggerCooldown = RuntimeStaggerCooldown;
        Context.Health = PredictedHealth;                 // predicted health // прогнозируемое здоровье
        Context.IsStagger = bStaggerOnCooldown;

        FPostDefenseOutput Out = AttackInfo.OptionalEnemyEffects->PostDefenseProcessing(Context);
        FinalHealthDamage = Out.NewFinalHealthDamage;
        StaggerMod = Out.NewStaggerChanceModifier;
        bForceStagger = Out.bNewForceStagger;
        RuntimeStaggerCooldown = Out.bNewForceStaggerCooldown;

    }

    FinalHealthDamage *= FinalHealthMod;
    Result.FinalHealthDamage = FinalHealthDamage;

    // 7. Subtract health
    // 7. Вычитание здоровья
    Health = FMath::Max(0.0f, Health - FinalHealthDamage);
    float HealthDelta = Health - OldHealth;
    Result.NewHealth = Health;

    TotalDamageDealt += FinalHealthDamage;
    Result.TotalDamageDealt = TotalDamageDealt;

    OnHealthChanged.Broadcast(Health, HealthDelta);

    if (FinalHealthDamage > 0.0f)
        LastHealthDamageTime = World->GetTimeSeconds();

    // 8. Determine zero-damage
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

    // 9. Stagger (using runtime parameters)
    // 9. Стаггер (используем runtime-параметры)
    if (Config && !bStaggerOnCooldown && !Result.bZeroDamage)
    {
        float StaggerDamage = 0.0f;
        switch (RuntimeStaggerInputType)
        {
        case EStaggerInputType::UseFinalHealthDamage:
            StaggerDamage = FinalHealthDamage;
            break;
        case EStaggerInputType::UseTotalDamageDealt:
            StaggerDamage = TotalDamageDealt;
            break;
        case EStaggerInputType::UseAttackStrength:
            StaggerDamage = DamageAfterZone;
            break;
        default:
            StaggerDamage = TotalDamageDealt;
        }

        float Chance = FMath::Clamp(GetStaggerChance(StaggerDamage) + StaggerMod, 0.0, 1.0);
        CurrentStaggerChance = Chance;
        if (bForceStagger || FMath::FRand() < Chance)
        {
            if (RuntimeStaggerCooldown > 0)
            {
                Result.bStaggerTriggered = true;
                bStaggerOnCooldown = true;
                OnStaggered.Broadcast();

                if (bForceStagger)
                {
                    CurrentStaggerChance = 1.0f;
                }

                World->GetTimerManager().SetTimer(StaggerCooldownTimer, this, &UEnemyDamageComponent::OnStaggerCooldownExpired, RuntimeStaggerCooldown, false);
            }
            else
            {
                if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Stagger triggered but RuntimeStaggerCooldown <= 0. Stagger will not be applied."));
                }
            }
        }
    }

    // 10. Update health zone (real state after subtraction)
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

    // 11. Check death
    // 11. Проверка смерти
    if (Health <= 0.0f && !bIsDead)
    {
        bIsDead = true;
        Result.bKilled = true;
        OnHealthDepleted.Broadcast(GetOwner(), Config ? Config->DeathBehaviorTag : NAME_None, AttackInfo.Instigator, AttackInfo.DamageSource);
        if (World)
        {
            World->GetTimerManager().ClearTimer(HealthRegenTimer);
            World->GetTimerManager().ClearTimer(ReserveRegenTimer);
        }
    }

    // 12. Debug log
    // 12. Отладочный лог
    DebugLogDamage(Result, ModifiedDamage, HitZoneMult, DamageAfterZone, FinalHealthDamage, HealthDelta, bAnyReserveChanged);

    // 13. Final event
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

    // Check for stagger
    // Проверка на стаггер
    if (bStaggerOnCooldown && !Params.bRegenWhileStaggered)
    {
        if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
        {
            AActor* Owner = GetOwner();
            FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        }
        return;
    }

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

        // Check for stagger for this layer
        // Проверка на стаггер для данного слоя
        if (bStaggerOnCooldown && !Params.bRegenWhileStaggered)
        {
            if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
            {
                AActor* Owner = GetOwner();
                FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
            }
            continue;
        }

        if (Params.RegenRatePerSecond <= 0.0f) continue;

        if (Params.bInterruptOnDamage && (World->GetTimeSeconds() - LastReserveDamageTimes[i]) < Params.RegenDelayAfterDamage)
            continue;

        float& Reserve = CurrentReserves[i];
        float OldReserve = Reserve;

        // Restore reserve up to the new maximum (RuntimeMaxReserves[i])
        // Восстанавливаем резерв до нового максимума (RuntimeMaxReserves[i])
        Reserve = FMath::Min(RuntimeMaxReserves[i], Reserve + Params.RegenRatePerSecond * 0.1f);

        float ActualDelta = Reserve - OldReserve;
        if (ActualDelta > 0.0f)
        {
            OnReserveChanged.Broadcast(Layer.LayerTag, Reserve);

            if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
            {
                AActor* Owner = GetOwner();
                FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
                UE_LOG(LogTemp, Log, TEXT("[%s] Reserve regen (%s): +%.1f (NewReserve %.1f / Max %.1f)"),
                    *OwnerName, *Layer.LayerTag.ToString(), ActualDelta, Reserve, RuntimeMaxReserves[i]);
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

    // If health does not fit into any zone (e.g., due to error) – take the last one (usually 0)
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

// ---- Health ----
// ---- Здоровье ----
void UEnemyDamageComponent::Heal(float Amount)
{
    if (bIsDead) return;
    if (Amount <= 0.0f) return;

    float OldHealth = Health;
    Health = FMath::Min(Health + Amount, MaxHealth);
    float Delta = Health - OldHealth;
    if (Delta > 0.0f)
    {
        OnHealthChanged.Broadcast(Health, Delta);
        // Update health zone
        // Обновляем зону здоровья
        FName OldZone = CurrentHealthZoneTag;
        UpdateHealthZone();
        if (OldZone != CurrentHealthZoneTag)
            OnHealthZoneChanged.Broadcast(CurrentHealthZoneTag, OldZone);
    }
}

void UEnemyDamageComponent::HealToMax(bool bInstant)
{
    if (bIsDead) return;
    float Missing = MaxHealth - Health;
    if (Missing > 0.0f)
        Heal(Missing);
}

void UEnemyDamageComponent::SetHealth(float NewHealth)
{
    if (bIsDead) return;

    float ClampedHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
    if (FMath::IsNearlyEqual(ClampedHealth, Health)) return;

    if (ClampedHealth <= 0.0f && !bIsDead)
    {
        Kill();
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

    // Scale zones proportionally
    // Масштабируем зоны пропорционально
    if (RuntimeHealthZones.Num() > 0 && OldMaxHealth > 0.0f)
    {
        float Scale = NewMaxHealth / OldMaxHealth;
        for (FHealthZoneDefinition& Zone : RuntimeHealthZones)
        {
            Zone.UpperBound *= Scale;
            Zone.LowerBound *= Scale;
        }
        CheckHealthZone(); // validation // валидация
    }

    // Adjust health if it exceeds the new maximum
    // Корректируем здоровье, если оно превышает новый максимум
    if (Health > MaxHealth)
    {
        Health = MaxHealth;
        OnHealthChanged.Broadcast(Health, 0.0f);
    }

    // Save the old zone, update, generate event on change
    // Сохраняем старую зону, обновляем, генерируем событие при изменении
    FName OldZone = CurrentHealthZoneTag;
    UpdateHealthZone();
    if (OldZone != CurrentHealthZoneTag)
    {
        OnHealthZoneChanged.Broadcast(CurrentHealthZoneTag, OldZone);

        // Logging
        // Логирование
        if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
        {
            AActor* Owner = GetOwner();
            FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
            UE_LOG(LogTemp, Log, TEXT("[%s] Health zone changed from %s to %s due to MaxHealth change (NewMax=%.1f)"),
                *OwnerName, *OldZone.ToString(), *CurrentHealthZoneTag.ToString(), MaxHealth);
        }
    }
}

// ---- Reserves ----
// ---- Резервы ----
void UEnemyDamageComponent::RestoreReserve(int32 LayerIndex, float Amount)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("RestoreReserve: Invalid LayerIndex %d"), LayerIndex);
        return;
    }

    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Reserve)
    {
        UE_LOG(LogTemp, Error, TEXT("RestoreReserve: Layer %d is not a Reserve layer"), LayerIndex);
        return;
    }

    if (bIsDead) return;

    float& Reserve = CurrentReserves[LayerIndex];
    float MaxRes = RuntimeMaxReserves[LayerIndex]; // <-- changed // <-- изменено
    float OldReserve = Reserve;
    Reserve = FMath::Min(Reserve + Amount, MaxRes);
    float Delta = Reserve - OldReserve;
    if (Delta > 0.0f)
    {
        OnReserveChanged.Broadcast(Config->DefenseLayers[LayerIndex].LayerTag, Reserve);
    }
}

void UEnemyDamageComponent::RestoreReserveToMax(int32 LayerIndex)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("RestoreReserveToMax: Invalid LayerIndex %d"), LayerIndex);
        return;
    }

    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Reserve)
    {
        UE_LOG(LogTemp, Error, TEXT("RestoreReserveToMax: Layer %d is not a Reserve layer"), LayerIndex);
        return;
    }

    float MaxRes = Config->DefenseLayers[LayerIndex].MaxReserve;
    float Current = CurrentReserves[LayerIndex];
    if (Current < MaxRes)
        RestoreReserve(LayerIndex, MaxRes - Current);
}

void UEnemyDamageComponent::SetReserve(int32 LayerIndex, float NewValue)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("SetReserve: Invalid LayerIndex %d"), LayerIndex);
        return;
    }

    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Reserve)
    {
        UE_LOG(LogTemp, Error, TEXT("SetReserve: Layer %d is not a Reserve layer"), LayerIndex);
        return;
    }

    if (bIsDead) return;

    float MaxRes = RuntimeMaxReserves[LayerIndex]; // <-- changed // <-- изменено
    float Clamped = FMath::Clamp(NewValue, 0.0f, MaxRes);
    if (FMath::IsNearlyEqual(Clamped, CurrentReserves[LayerIndex])) return;

    CurrentReserves[LayerIndex] = Clamped;
    OnReserveChanged.Broadcast(Config->DefenseLayers[LayerIndex].LayerTag, Clamped);
}

// ---- Reserve regeneration ----
// ---- Регенерация резервов ----
void UEnemyDamageComponent::SetAllReserveRegenEnabled(bool bEnabled)
{
    if (!Config) return;

    int32 ModifiedCount = 0;
    for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
    {
        if (Config->DefenseLayers[i].LayerType == EDefenseLayerType::Reserve)
        {
            if (bReserveRegenEnabled.IsValidIndex(i))
            {
                if (bReserveRegenEnabled[i] != bEnabled)
                {
                    bReserveRegenEnabled[i] = bEnabled;
                    ModifiedCount++;
                }
            }
        }
    }

    // Logging
    // Логирование
    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[%s] Set reserve regeneration %s for %d layers"),
            *OwnerName, bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"), ModifiedCount);
    }
}

void UEnemyDamageComponent::SetReserveRegenEnabled(int32 LayerIndex, bool bEnabled)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex)) return;
    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Reserve) return;
    if (!bReserveRegenEnabled.IsValidIndex(LayerIndex)) return;

    if (bReserveRegenEnabled[LayerIndex] != bEnabled)
    {
        bReserveRegenEnabled[LayerIndex] = bEnabled;

        if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
        {
            AActor* Owner = GetOwner();
            FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
            UE_LOG(LogTemp, Log, TEXT("[%s] Reserve regeneration for layer %d (%s) set to %s"),
                *OwnerName, LayerIndex, *Config->DefenseLayers[LayerIndex].LayerTag.ToString(),
                bEnabled ? TEXT("ENABLED") : TEXT("DISABLED"));
        }
    }
}

void UEnemyDamageComponent::SetMaxReserve(int32 LayerIndex, float NewMaxReserve)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("SetMaxReserve: Invalid LayerIndex %d"), LayerIndex);
        return;
    }

    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Reserve)
    {
        UE_LOG(LogTemp, Error, TEXT("SetMaxReserve: Layer %d is not a Reserve layer"), LayerIndex);
        return;
    }

    if (!RuntimeMaxReserves.IsValidIndex(LayerIndex)) return;

    float OldMax = RuntimeMaxReserves[LayerIndex];
    RuntimeMaxReserves[LayerIndex] = FMath::Max(0.0f, NewMaxReserve);

    // If the current reserve exceeds the new maximum, trim it
    // Если текущий резерв превышает новый максимум, обрезаем его
    if (CurrentReserves[LayerIndex] > RuntimeMaxReserves[LayerIndex])
    {
        CurrentReserves[LayerIndex] = RuntimeMaxReserves[LayerIndex];
        OnReserveChanged.Broadcast(Config->DefenseLayers[LayerIndex].LayerTag, CurrentReserves[LayerIndex]);
    }

    // Logging
    // Логирование
    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[%s] MaxReserve for layer [%s] (index %d) changed from %.1f to %.1f"),
            *OwnerName, *Config->DefenseLayers[LayerIndex].LayerTag.ToString(), LayerIndex, OldMax, RuntimeMaxReserves[LayerIndex]);
    }
}

void UEnemyDamageComponent::SetReserveRegenPerSecond(int32 LayerIndex, float Rate)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("SetReserveRegenRate: Invalid LayerIndex %d"), LayerIndex);
        return;
    }

    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Reserve)
    {
        UE_LOG(LogTemp, Error, TEXT("SetReserveRegenRate: Layer %d is not a Reserve layer"), LayerIndex);
        return;
    }

    if (!RuntimeReserveRegenParams.IsValidIndex(LayerIndex)) return;

    float OldSpeed = RuntimeReserveRegenParams[LayerIndex].RegenRatePerSecond;
    RuntimeReserveRegenParams[LayerIndex].RegenRatePerSecond = FMath::Max(0.0f, Rate);

    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[%s] Set Reserve Regen Per Second for layer [%s] (index %d) changed from %.1f to %.1f"),
            *OwnerName, *Config->DefenseLayers[LayerIndex].LayerTag.ToString(), LayerIndex, OldSpeed, RuntimeReserveRegenParams[LayerIndex].RegenRatePerSecond);
    }
}

void UEnemyDamageComponent::SetReserveRegenDelay(int32 LayerIndex, float Delay)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("SetReserveRegenDelay: Invalid LayerIndex %d"), LayerIndex);
        return;
    }

    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Reserve)
    {
        UE_LOG(LogTemp, Error, TEXT("SetReserveRegenDelay: Layer %d is not a Reserve layer"), LayerIndex);
        return;
    }

    if (!RuntimeReserveRegenParams.IsValidIndex(LayerIndex)) return;

    float OldDelay = RuntimeReserveRegenParams[LayerIndex].RegenDelayAfterDamage;
    RuntimeReserveRegenParams[LayerIndex].RegenDelayAfterDamage = FMath::Max(0.0f, Delay);

    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[%s] Set Reserve Regen Delay for layer [%s] (index %d) changed from %.1f to %.1f"),
            *OwnerName, *Config->DefenseLayers[LayerIndex].LayerTag.ToString(), LayerIndex, OldDelay, RuntimeReserveRegenParams[LayerIndex].RegenDelayAfterDamage);
    }
}

void UEnemyDamageComponent::SetReserveRegenInterruptOnDamage(int32 LayerIndex, bool bInterrupt)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("SetReserveRegenInterruptOnDamage: Invalid LayerIndex %d"), LayerIndex);
        return;
    }

    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Reserve)
    {
        UE_LOG(LogTemp, Error, TEXT("SetReserveRegenInterruptOnDamage: Layer %d is not a Reserve layer"), LayerIndex);
        return;
    }

    if (!RuntimeReserveRegenParams.IsValidIndex(LayerIndex)) return;

    RuntimeReserveRegenParams[LayerIndex].bInterruptOnDamage = bInterrupt;

    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[%s] Set Reserve Regen Interrupt On Damage for layer [%s] (index %d) changed to %s"),
            *OwnerName, *Config->DefenseLayers[LayerIndex].LayerTag.ToString(), LayerIndex, bInterrupt ? TEXT("true") : TEXT("false"));
    }
}

// ---- Management of reserve regeneration: while staggered flag (by index) ----
// ---- Управление регенерацией резервов: флаг while staggered (по индексу) ----
void UEnemyDamageComponent::SetReserveRegenWhileStaggered(int32 LayerIndex, bool bAllow)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("SetReserveRegenWhileStaggered: Invalid LayerIndex %d"), LayerIndex);
        return;
    }

    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Reserve)
    {
        UE_LOG(LogTemp, Error, TEXT("SetReserveRegenWhileStaggered: Layer %d is not a Reserve layer"), LayerIndex);
        return;
    }

    if (!RuntimeReserveRegenParams.IsValidIndex(LayerIndex)) return;

    bool OldValue = RuntimeReserveRegenParams[LayerIndex].bRegenWhileStaggered;
    if (OldValue == bAllow) return;

    RuntimeReserveRegenParams[LayerIndex].bRegenWhileStaggered = bAllow;

    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[%s] Reserve regen while staggered for layer [%s] (index %d) set to %s (was %s)"),
            *OwnerName, *Config->DefenseLayers[LayerIndex].LayerTag.ToString(), LayerIndex,
            bAllow ? TEXT("true") : TEXT("false"), OldValue ? TEXT("true") : TEXT("false"));
    }
}

// ---- Management of reserve regeneration: while staggered flag (for all) ----
// ---- Управление регенерацией резервов: флаг while staggered (для всех) ----
void UEnemyDamageComponent::SetAllReserveRegenWhileStaggered(bool bAllow)
{
    if (!Config) return;

    int32 ModifiedCount = 0;
    for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
    {
        if (Config->DefenseLayers[i].LayerType == EDefenseLayerType::Reserve)
        {
            if (RuntimeReserveRegenParams.IsValidIndex(i))
            {
                bool OldValue = RuntimeReserveRegenParams[i].bRegenWhileStaggered;
                if (OldValue != bAllow)
                {
                    RuntimeReserveRegenParams[i].bRegenWhileStaggered = bAllow;
                    ModifiedCount++;
                }
            }
        }
    }

    if (ModifiedCount > 0 && CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[%s] Reserve regen while staggered set to %s for %d layers"),
            *OwnerName, bAllow ? TEXT("true") : TEXT("false"), ModifiedCount);
    }
}

// ---- Resistance ----
// ---- Сопротивление ----
void UEnemyDamageComponent::SetResistanceMultiplier(int32 LayerIndex, float NewMultiplier)
{
    if (!Config || !Config->DefenseLayers.IsValidIndex(LayerIndex))
    {
        UE_LOG(LogTemp, Error, TEXT("SetResistanceMultiplier: Invalid LayerIndex %d"), LayerIndex);
        return;
    }

    if (Config->DefenseLayers[LayerIndex].LayerType != EDefenseLayerType::Resistance)
    {
        UE_LOG(LogTemp, Error, TEXT("SetResistanceMultiplier: Layer %d is not a Resistance layer"), LayerIndex);
        return;
    }

    if (!RuntimeResistanceMultipliers.IsValidIndex(LayerIndex)) return;

    float OldResistence = RuntimeResistanceMultipliers[LayerIndex];
    RuntimeResistanceMultipliers[LayerIndex] = FMath::Max(0.0f, NewMultiplier);

    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[%s] Set Resistance Multiplier for layer [%s] (index %d) changed from %.1f to %.1f"),
            *OwnerName, *Config->DefenseLayers[LayerIndex].LayerTag.ToString(), LayerIndex, OldResistence, RuntimeResistanceMultipliers[LayerIndex]);
    }
}

// ---- Health regeneration ----
// ---- Регенерация здоровья ----
void UEnemyDamageComponent::SetHealthRegenEnabled(bool bEnabled)
{
    bHealthRegenEnabled = bEnabled;
    if (!bEnabled)
    {
        UWorld* World = GetWorld();
        if (World)
            World->GetTimerManager().ClearTimer(HealthRegenTimer);

        if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
        {
            AActor* Owner = GetOwner();
            FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
            UE_LOG(LogTemp, Log, TEXT("[%s] Set Health Regen Enabled true"),
                *OwnerName);
        }
    }
    else
    {
        // Restart the timer if it is not running
        // Перезапустить таймер, если он не работает
        UWorld* World = GetWorld();
        if (World && Config && RuntimeHealthRegenParams.RegenRatePerSecond > 0.0f)
        {
            World->GetTimerManager().SetTimer(HealthRegenTimer, this, &UEnemyDamageComponent::ApplyHealthRegen, 0.1f, true);
        }

        if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
        {
            AActor* Owner = GetOwner();
            FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
            UE_LOG(LogTemp, Log, TEXT("[%s] Set Health Regen Enabled false"),
                *OwnerName);
        }
    }
}

void UEnemyDamageComponent::SetHealthRegenRatePerSecond(float Rate)
{
    float OldRate = RuntimeHealthRegenParams.RegenRatePerSecond;
    RuntimeHealthRegenParams.RegenRatePerSecond = FMath::Max(0.0f, Rate);
    // If regeneration is enabled and the timer is not running – restart it
    // Если регенерация включена и таймер не работает – перезапустить
    if (bHealthRegenEnabled && Rate > 0.0f)
    {
        UWorld* World = GetWorld();
        if (World && !World->GetTimerManager().IsTimerActive(HealthRegenTimer))
        {
            World->GetTimerManager().SetTimer(HealthRegenTimer, this, &UEnemyDamageComponent::ApplyHealthRegen, 0.1f, true);
        }
    }

    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[%s] Set Health Regen Rate Per Second changed from %.1f to %.1f"),
            *OwnerName, OldRate, RuntimeHealthRegenParams.RegenRatePerSecond);
    }
}

void UEnemyDamageComponent::SetHealthRegenDelay(float Delay)
{
    float OldDelay = RuntimeHealthRegenParams.RegenDelayAfterDamage;
    RuntimeHealthRegenParams.RegenDelayAfterDamage = FMath::Max(0.0f, Delay);

    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[%s] Set Health Regen Delay changed from %.1f to %.1f"),
            *OwnerName, OldDelay, RuntimeHealthRegenParams.RegenDelayAfterDamage);
    }
}

void UEnemyDamageComponent::SetHealthRegenInterruptOnDamage(bool bInterrupt)
{
    bool OldInterrupt = RuntimeHealthRegenParams.bInterruptOnDamage;
    RuntimeHealthRegenParams.bInterruptOnDamage = bInterrupt;

    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[%s] Set HealthRegenInterruptOnDamage from %s to %s"),
            *OwnerName, OldInterrupt ? TEXT("true") : TEXT("false"), RuntimeHealthRegenParams.bInterruptOnDamage ? TEXT("true") : TEXT("false"));
    }
}

void UEnemyDamageComponent::SetHealthZones(const TArray<FHealthZoneDefinition>& NewZones)
{
    RuntimeHealthZones = NewZones;
    // Sort in descending order of UpperBound (as in the config)
    // Сортируем по убыванию UpperBound (как в конфиге)
    RuntimeHealthZones.Sort([](const FHealthZoneDefinition& A, const FHealthZoneDefinition& B) {
        return A.UpperBound > B.UpperBound;
        });
    CheckHealthZone();

    FName OldZone = CurrentHealthZoneTag;
    UpdateHealthZone();

    // Debug print (if enabled)
    // Отладочная печать (если включена)
    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[%s] Health zones updated. New zones count: %d"), *OwnerName, RuntimeHealthZones.Num());
        // Print zone boundaries
        // Вывод границ зон
        for (int32 i = 0; i < RuntimeHealthZones.Num(); ++i)
        {
            const FHealthZoneDefinition& Zone = RuntimeHealthZones[i];
            UE_LOG(LogTemp, Log, TEXT("[%s]   Zone %d: %s (%.1f - %.1f]"),
                *OwnerName, i, *Zone.ZoneTag.ToString(), Zone.LowerBound, Zone.UpperBound);
        }
        if (OldZone != CurrentHealthZoneTag)
        {
            UE_LOG(LogTemp, Log, TEXT("[%s]   Current zone changed from %s to %s"),
                *OwnerName, *OldZone.ToString(), *CurrentHealthZoneTag.ToString());
        }
    }
}

// ---- Management of health regeneration: while staggered flag ----
// ---- Управление регенерацией здоровья: флаг while staggered ----
void UEnemyDamageComponent::SetHealthRegenWhileStaggered(bool bAllow)
{
    if (!Config) return;

    bool OldValue = RuntimeHealthRegenParams.bRegenWhileStaggered;
    if (OldValue == bAllow) return;

    RuntimeHealthRegenParams.bRegenWhileStaggered = bAllow;

    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[%s] Health regen while staggered set to %s (was %s)"),
            *OwnerName, bAllow ? TEXT("true") : TEXT("false"), OldValue ? TEXT("true") : TEXT("false"));
    }
}

// ---- Stagger ----
// ---- Стаггер ----
void UEnemyDamageComponent::SetStaggerParams(float BaseChance, float Susceptibility, float Cooldown, EStaggerInputType InputType)
{
    RuntimeBaseStaggerChance = FMath::Clamp(BaseChance, 0.0f, 1.0f);
    RuntimeStaggerSusceptibility = FMath::Max(0.0f, Susceptibility);
    RuntimeStaggerCooldown = FMath::Max(0.0f, Cooldown);
    RuntimeStaggerInputType = InputType;

    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
    {
        AActor* Owner = GetOwner();

        FString InputTypeStr;

        switch (InputType)
        {
        case EStaggerInputType::UseFinalHealthDamage:
        {
            InputTypeStr = TEXT("UseFinalHealthDamage");
            break;
        }
        case EStaggerInputType::UseTotalDamageDealt:
        {
            InputTypeStr = TEXT("UseTotalDamageDealt");
            break;
        }
        case EStaggerInputType::UseAttackStrength:
        {
            InputTypeStr = TEXT("UseAttackStrength");
            break;
        }
        }

        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[%s] Stagger params updated: BaseChance=%.2f, Susceptibility=%.2f, Cooldown=%.2f, InputType=%s"),
            *OwnerName, RuntimeBaseStaggerChance, RuntimeStaggerSusceptibility, RuntimeStaggerCooldown, *InputTypeStr);
    }
}

// ---- Forced death ----
// ---- Принудительная смерть ----
void UEnemyDamageComponent::Kill()
{
    if (bIsDead) return;
    if (Health <= 0.0f) return; // already dead // уже мёртв

    // Save old health for delta
    // Сохраняем старое здоровье для дельты
    float OldHealth = Health;

    // Set health to 0 and death flag
    // Устанавливаем здоровье в 0 и флаг смерти
    Health = 0.0f;
    bIsDead = true;
    // Signal health change (delta = 0 - OldHealth)
    // Сигнал об изменении здоровья (дельта = 0 - OldHealth)
    OnHealthChanged.Broadcast(Health, -OldHealth);

    // Update health zone (now it should become the last one, usually "Low" or "Dead")
    // Обновляем зону здоровья (теперь она должна стать последней, обычно "Low" или "Dead")
    FName OldZone = CurrentHealthZoneTag;
    UpdateHealthZone();
    if (OldZone != CurrentHealthZoneTag)
    {
        OnHealthZoneChanged.Broadcast(CurrentHealthZoneTag, OldZone);
    }

    // Stop regeneration
    // Останавливаем регенерацию
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(HealthRegenTimer);
        World->GetTimerManager().ClearTimer(ReserveRegenTimer);
    }

    // Death signal
    // Сигнал о смерти
    OnHealthDepleted.Broadcast(GetOwner(), Config ? Config->DeathBehaviorTag : NAME_None, nullptr, nullptr);
    OnSuicide.Broadcast();

    // Debug log (if enabled)
    // Отладочный лог (если включён)
    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() != 0)
    {
        AActor* Owner = GetOwner();
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Log, TEXT("[%s] Killed by script."), *OwnerName);
    }
}

void UEnemyDamageComponent::DebugLogDamage(const FDamageResult& Result, float ModifiedDamage, float HitZoneMult, float DamageAfterZone, float FinalHealthDamage, float HealthDelta, bool bAnyReserveChanged)
{
    if (CVarEnemyDamageVerboseLogging.GetValueOnGameThread() == 0)
        return;

    AActor* Owner = GetOwner();
    FString OwnerName = Owner ? Owner->GetName() : TEXT("None");

    FString LogString;
    LogString += FString::Printf(TEXT("[%s] Incoming %.1f"), *OwnerName, ModifiedDamage);

    LogString += CurrentHitZoneName.IsNone() ? FString::Printf(TEXT(" -> HitRegion x%.2f (%.1f)"), HitZoneMult, DamageAfterZone) : FString::Printf(TEXT(" -> HitRegion [%s] x%.2f (%.1f)"), *CurrentHitZoneName.ToString(), HitZoneMult, DamageAfterZone);

    if (Config && Config->DefenseLayers.Num() > 0)
    {
        for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
        {
            const FDefenseLayer& Layer = Config->DefenseLayers[i];
            if (Layer.LayerType == EDefenseLayerType::Resistance)
            {
                float absorbed = Result.LayerAbsorbedDamage[i];
                if (absorbed > 0.0f)
                    LogString += FString::Printf(TEXT(" -> Resistance [Tag: %s] x%.2f (absorbed %.1f)"), *Layer.LayerTag.ToString(), RuntimeResistanceMultipliers[i]/*Layer.ResistanceMultiplier*/, absorbed);
                else
                    LogString += FString::Printf(TEXT(" -> Resistance [Tag: %s] x%.2f"), *Layer.LayerTag.ToString(), RuntimeResistanceMultipliers[i]);
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

    if (Config && RuntimeHealthZones.Num() > 0)
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