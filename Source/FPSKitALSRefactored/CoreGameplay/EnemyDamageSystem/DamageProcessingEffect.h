#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DamageProcessing.h"
#include "DamageProcessingEffect.generated.h"

UCLASS(Abstract, Blueprintable, EditInlineNew)
class UDamageProcessingEffect : public UDataAsset
{
    GENERATED_BODY()

public:
    // Изменяет входящий урон до защиты. Возвращает новый урон.
    UFUNCTION(BlueprintNativeEvent, Category = "Damage Processing")
    float ModifyDamageProcessing(float IncomingDamage);

    // Изменяет массивы модификаторов защиты. Принимает текущие значения, возвращает новые.
    UFUNCTION(BlueprintNativeEvent, Category = "Damage Processing")
    FDefenseModifiers ApplyDefenseModifiers(const FDefenseModifiers& InModifiers);

    // Изменяет финальный урон по здоровью и параметры стаггера.
    UFUNCTION(BlueprintNativeEvent, Category = "Damage Processing")
    FPostDefenseResult PostDefenseProcessing(const FPostDefenseResult& InPostResult);

    // Реализации по умолчанию (возвращают входные значения без изменений)
    virtual float ModifyDamageProcessing_Implementation(float IncomingDamage) { return IncomingDamage; }
    virtual FDefenseModifiers ApplyDefenseModifiers_Implementation(const FDefenseModifiers& InModifiers) { return InModifiers; }
    virtual FPostDefenseResult PostDefenseProcessing_Implementation(const FPostDefenseResult& InPostResult) { return InPostResult; }
};