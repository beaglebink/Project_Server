#include "EnemyDamageBlueprintLibrary.h"
#include "EnemyDamageConfig.h"
#include "Engine/Engine.h"

// ==================== Setting by index ====================
// ==================== Установка по индексу ====================

TArray<bool> UEnemyDamageBlueprintLibrary::SetBoolArrayValue(const TArray<bool>& Source, int32 Index, bool Value, AActor* Owner, AActor* Weapon)
{
    TArray<bool> Result = Source;
    if (Result.IsValidIndex(Index))
    {
        Result[Index] = Value;

        if (Value)
        {
            LogModification(Owner, Weapon,
                FString::Printf(TEXT("IgnoreLayer [%i] set to %s (was %s)"),
                    Index,
                    Value ? TEXT("true") : TEXT("false"),
                    !Value ? TEXT("true") : TEXT("false")
                )
            );
        }
    }
    else
    {
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Error, TEXT("Owner %s Incorrect implementation of ignoring defense layers. Incorrect number of layers (%i)."), *OwnerName, Result.Num());
    }


    return Result;
}

TArray<float> UEnemyDamageBlueprintLibrary::SetFloatArrayValue(const TArray<float>& Source, int32 Index, float Value, AActor* Owner, AActor* Weapon)
{
    TArray<float> Result = Source;
    if (Result.IsValidIndex(Index))
    {
        Result[Index] = Value;

        float OldValue = Result[Index];

        LogModification(Owner, Weapon,
            FString::Printf(TEXT("Modify Layer [%i] changed from %.2f to %.2f"),
                Index, OldValue, Value)
        );
    }
    else
    {
        FString OwnerName = Owner ? Owner->GetName() : TEXT("None");
        UE_LOG(LogTemp, Error, TEXT("Owner %s Incorrect implementation of ignoring defense layers. Incorrect number of layers (%i)."), *OwnerName, Result.Num());
    }

    return Result;
}

// ==================== Modification by tag ====================
// ==================== Модификация по тегу ====================

TArray<bool> UEnemyDamageBlueprintLibrary::SetIgnoreLayerByTag(
    const TArray<bool>& Source,
    const UEnemyDamageConfig* Config,
    FName LayerTag,
    bool bIgnore,
    AActor* Owner,
    AActor* Weapon)
{
    TArray<bool> Result = Source;
    if (!Config) return Result;

    for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
    {
        if (Config->DefenseLayers[i].LayerTag == LayerTag)
        {
            if (Result.IsValidIndex(i))
            {
                bool OldValue = Result[i];
                Result[i] = bIgnore;
                if (OldValue != bIgnore)
                {
                    LogModification(Owner, Weapon,
                        FString::Printf(TEXT("IgnoreLayer [%s] set to %s (was %s)"),
                            *LayerTag.ToString(),
                            bIgnore ? TEXT("true") : TEXT("false"),
                            OldValue ? TEXT("true") : TEXT("false")
                        )
                    );
                }
            }
            break;
        }
    }
    return Result;
}

TArray<float> UEnemyDamageBlueprintLibrary::SetResistanceMultiplierByTag(
    const TArray<float>& Source,
    const UEnemyDamageConfig* Config,
    FName LayerTag,
    float NewMultiplier,
    AActor* Owner,
    AActor* Weapon)
{
    TArray<float> Result = Source;
    if (!Config) return Result;

    for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
    {
        if (Config->DefenseLayers[i].LayerTag == LayerTag &&
            Config->DefenseLayers[i].LayerType == EDefenseLayerType::Resistance)
        {
            if (Result.IsValidIndex(i))
            {
                float OldValue = Result[i];
                Result[i] = NewMultiplier;
                if (!FMath::IsNearlyEqual(OldValue, NewMultiplier, 0.001f))
                {
                    LogModification(Owner, Weapon,
                        FString::Printf(TEXT("ResistanceMultiplier [%s] changed from %.2f to %.2f"),
                            *LayerTag.ToString(), OldValue, NewMultiplier)
                    );
                }
            }
            break;
        }
    }
    return Result;
}

TArray<float> UEnemyDamageBlueprintLibrary::SetReserveDamageMultiplierByTag(
    const TArray<float>& Source,
    const UEnemyDamageConfig* Config,
    FName LayerTag,
    float NewMultiplier,
    AActor* Owner,
    AActor* Weapon)
{
    TArray<float> Result = Source;
    if (!Config) return Result;

    for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
    {
        if (Config->DefenseLayers[i].LayerTag == LayerTag &&
            Config->DefenseLayers[i].LayerType == EDefenseLayerType::Reserve)
        {
            if (Result.IsValidIndex(i))
            {
                float OldValue = Result[i];
                Result[i] = NewMultiplier;
                if (!FMath::IsNearlyEqual(OldValue, NewMultiplier, 0.001f))
                {
                    LogModification(Owner, Weapon,
                        FString::Printf(TEXT("ReserveDamageMultiplier [%s] changed from %.2f to %.2f"),
                            *LayerTag.ToString(), OldValue, NewMultiplier)
                    );
                }
            }
            break;
        }
    }
    return Result;
}

// ==================== Ignoring all layers of a type ====================
// ==================== Игнорирование всех слоёв типа ====================

TArray<bool> UEnemyDamageBlueprintLibrary::IgnoreLayersOfType(
    const TArray<bool>& Source,
    const UEnemyDamageConfig* Config,
    EDefenseLayerType LayerType,
    bool bIgnore,
    AActor* Owner,
    AActor* Weapon)
{
    TArray<bool> Result = Source;
    if (!Config) return Result;

    int32 ModifiedCount = 0;
    int32 TotalCount = 0;
    for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
    {
        if (Config->DefenseLayers[i].LayerType == LayerType)
        {
            TotalCount++;
            if (Result.IsValidIndex(i))
            {
                bool OldValue = Result[i];
                Result[i] = bIgnore;
                if (OldValue != bIgnore)
                    ModifiedCount++;
            }
        }
    }

    if (ModifiedCount > 0)
    {
        FString TypeName = (LayerType == EDefenseLayerType::Resistance) ? TEXT("Resistance") : TEXT("Reserve");
        LogModification(Owner, Weapon,
            FString::Printf(TEXT("Ignored %d of %d %s layers (set IgnoreLayer = %s)"),
                ModifiedCount, TotalCount, *TypeName, bIgnore ? TEXT("true") : TEXT("false"))
        );
    }
    return Result;
}

// ==================== Logging ====================
// ==================== Логирование ====================

void UEnemyDamageBlueprintLibrary::LogModification(
    AActor* Owner,
    AActor* Weapon,
    const FString& Message)
{
    FString OwnerName = Owner ? Owner->GetName() : TEXT("Unknown");
    FString WeaponName = Weapon ? Weapon->GetName() : TEXT("Unknown");
    FString FullMessage = FString::Printf(TEXT("[%s] [%s] %s"), *OwnerName, *WeaponName, *Message);

    // Log to Output Log
    // Лог в Output Log
    UE_LOG(LogTemp, Log, TEXT("%s"), *FullMessage);

    // Display on screen (if GEngine is enabled)
    // Вывод на экран (если включён GEngine)
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, FullMessage);
    }
}

// ==================== Bulk modifications ====================
// ==================== Массовые модификации ====================

TArray<bool> UEnemyDamageBlueprintLibrary::SetIgnoreLayerForAllResistance(
    const TArray<bool>& Source,
    const UEnemyDamageConfig* Config,
    bool bIgnore,
    AActor* Owner,
    AActor* Weapon)
{
    TArray<bool> Result = Source;
    if (!Config) return Result;

    int32 ModifiedCount = 0;
    for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
    {
        if (Config->DefenseLayers[i].LayerType == EDefenseLayerType::Resistance)
        {
            if (Result.IsValidIndex(i))
            {
                bool OldValue = Result[i];
                Result[i] = bIgnore;
                if (OldValue != bIgnore)
                    ModifiedCount++;
            }
        }
    }

    if (ModifiedCount > 0)
    {
        LogModification(Owner, Weapon,
            FString::Printf(TEXT("Set IgnoreLayer for all Resistance layers to %s (modified %d layers)"),
                bIgnore ? TEXT("true") : TEXT("false"), ModifiedCount)
        );
    }
    return Result;
}

TArray<bool> UEnemyDamageBlueprintLibrary::SetIgnoreLayerForAllReserves(
    const TArray<bool>& Source,
    const UEnemyDamageConfig* Config,
    bool bIgnore,
    AActor* Owner,
    AActor* Weapon)
{
    TArray<bool> Result = Source;
    if (!Config) return Result;

    int32 ModifiedCount = 0;
    for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
    {
        if (Config->DefenseLayers[i].LayerType == EDefenseLayerType::Reserve)
        {
            if (Result.IsValidIndex(i))
            {
                bool OldValue = Result[i];
                Result[i] = bIgnore;
                if (OldValue != bIgnore)
                    ModifiedCount++;
            }
        }
    }

    if (ModifiedCount > 0)
    {
        LogModification(Owner, Weapon,
            FString::Printf(TEXT("Set IgnoreLayer for all Reserve layers to %s (modified %d layers)"),
                bIgnore ? TEXT("true") : TEXT("false"), ModifiedCount)
        );
    }
    return Result;
}

TArray<float> UEnemyDamageBlueprintLibrary::SetResistanceMultiplierForAllResistance(
    const TArray<float>& Source,
    const UEnemyDamageConfig* Config,
    float NewMultiplier,
    AActor* Owner,
    AActor* Weapon)
{
    TArray<float> Result = Source;
    if (!Config) return Result;

    int32 ModifiedCount = 0;
    for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
    {
        if (Config->DefenseLayers[i].LayerType == EDefenseLayerType::Resistance)
        {
            if (Result.IsValidIndex(i))
            {
                float OldValue = Result[i];
                Result[i] = NewMultiplier;
                if (!FMath::IsNearlyEqual(OldValue, NewMultiplier, 0.001f))
                    ModifiedCount++;
            }
        }
    }

    if (ModifiedCount > 0)
    {
        LogModification(Owner, Weapon,
            FString::Printf(TEXT("Set ResistanceMultiplier for all Resistance layers to %.2f (modified %d layers)"),
                NewMultiplier, ModifiedCount)
        );
    }
    return Result;
}

TArray<float> UEnemyDamageBlueprintLibrary::SetReserveDamageMultiplierForAllReserves(
    const TArray<float>& Source,
    const UEnemyDamageConfig* Config,
    float NewMultiplier,
    AActor* Owner,
    AActor* Weapon)
{
    TArray<float> Result = Source;
    if (!Config) return Result;

    int32 ModifiedCount = 0;
    for (int32 i = 0; i < Config->DefenseLayers.Num(); ++i)
    {
        if (Config->DefenseLayers[i].LayerType == EDefenseLayerType::Reserve)
        {
            if (Result.IsValidIndex(i))
            {
                float OldValue = Result[i];
                Result[i] = NewMultiplier;
                if (!FMath::IsNearlyEqual(OldValue, NewMultiplier, 0.001f))
                    ModifiedCount++;
            }
        }
    }

    if (ModifiedCount > 0)
    {
        LogModification(Owner, Weapon,
            FString::Printf(TEXT("Set ReserveDamageMultiplier for all Reserve layers to %.2f (modified %d layers)"),
                NewMultiplier, ModifiedCount)
        );
    }
    return Result;
}