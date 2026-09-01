#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EnemyDamageTypes.h"
#include "EnemyDamageBlueprintLibrary.generated.h"

UCLASS()
class UEnemyDamageBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Sets a value in the array by index (with bounds checking)
    // Устанавливает значение в массиве по индексу (с проверкой границ)
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Utilities")
    static TArray<bool> SetBoolArrayValue(const TArray<bool>& Source, int32 Index, bool Value, AActor* Owner, AActor* Weapon);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Utilities")
    static TArray<float> SetFloatArrayValue(const TArray<float>& Source, int32 Index, float Value, AActor* Owner, AActor* Weapon);

    // Modifies the IgnoreLayer array for the layer with the given tag (requires Config)
    // Изменяет массив IgnoreLayer для слоя с заданным тегом (нужен Config)
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Utilities")
    static TArray<bool> SetIgnoreLayerByTag(const TArray<bool>& Source, const UEnemyDamageConfig* Config, FName LayerTag, bool bIgnore, AActor* Owner, AActor* Weapon);

    // Modifies the ResistanceMultipliers array by tag
    // Изменяет массив ResistanceMultipliers по тегу
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Utilities")
    static TArray<float> SetResistanceMultiplierByTag(const TArray<float>& Source, const UEnemyDamageConfig* Config, FName LayerTag, float NewMultiplier, AActor* Owner, AActor* Weapon);

    // Modifies the ReserveDamageMultipliers array by tag
    // Изменяет массив ReserveDamageMultipliers по тегу
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Utilities")
    static TArray<float> SetReserveDamageMultiplierByTag(const TArray<float>& Source, const UEnemyDamageConfig* Config, FName LayerTag, float NewMultiplier, AActor* Owner, AActor* Weapon);

    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Utilities")
    static TArray<bool> IgnoreLayersOfType(const TArray<bool>& Source, const UEnemyDamageConfig* Config, EDefenseLayerType LayerType, bool bIgnore, AActor* Owner, AActor* Weapon);

    // Sets IgnoreLayer = bIgnore for all layers of type Resistance
    // Устанавливает IgnoreLayer = bIgnore для всех слоёв типа Resistance
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Utilities")
    static TArray<bool> SetIgnoreLayerForAllResistance(const TArray<bool>& Source, const UEnemyDamageConfig* Config, bool bIgnore, AActor* Owner, AActor* Weapon);

    // Sets IgnoreLayer = bIgnore for all layers of type Reserve
    // Устанавливает IgnoreLayer = bIgnore для всех слоёв типа Reserve
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Utilities")
    static TArray<bool> SetIgnoreLayerForAllReserves(const TArray<bool>& Source, const UEnemyDamageConfig* Config, bool bIgnore, AActor* Owner, AActor* Weapon);

    // Sets the multiplier for all Resistance layers
    // Устанавливает множитель для всех слоёв Resistance
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Utilities")
    static TArray<float> SetResistanceMultiplierForAllResistance(const TArray<float>& Source, const UEnemyDamageConfig* Config, float NewMultiplier, AActor* Owner, AActor* Weapon);

    // Sets the reserve damage multiplier for all Reserve layers
    // Устанавливает множитель урона по резерву для всех слоёв Reserve
    UFUNCTION(BlueprintCallable, Category = "Enemy Damage|Utilities")
    static TArray<float> SetReserveDamageMultiplierForAllReserves(const TArray<float>& Source, const UEnemyDamageConfig* Config, float NewMultiplier, AActor* Owner, AActor* Weapon);

    static void LogModification(AActor* Owner, AActor* Weapon, const FString& Message);
};