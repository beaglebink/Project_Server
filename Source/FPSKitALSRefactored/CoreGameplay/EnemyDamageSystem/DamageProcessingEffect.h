#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DamageProcessing.h"
#include "DamageProcessingEffect.generated.h"

UCLASS(Abstract, Blueprintable, EditInlineNew)
class UDamageProcessingEffect : public UObject
{
    GENERATED_BODY()

public:
    // Вызывается перед применением слоёв защиты.
    // Может изменять IncomingDamage, временно подменять сопротивление/резервы и т.д.
    virtual void ModifyDamageProcessing(FDamageProcessingContext& Context) {}

    // Вызывается после прохождения всех слоёв, но до вычитания здоровья.
    virtual void PostDefenseProcessing(FDamageProcessingContext& Context, float& FinalHealthDamage) {}
};