// DamageProcessingEffect.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DamageProcessing.h"

// бяецдю онякедмхл!
#include "DamageProcessingEffect.generated.h"

UCLASS(Abstract, Blueprintable, EditInlineNew)
class UDamageProcessingEffect : public UDataAsset
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, Category = "Damage Processing")
    FPreDefenseOutput ModifyDamageProcessing(const FDamageProcessingContext& Context);

    UFUNCTION(BlueprintNativeEvent, Category = "Damage Processing")
    FDefenseOutput ApplyDefenseModifiers(const FDamageProcessingContext& Context);

    UFUNCTION(BlueprintNativeEvent, Category = "Damage Processing")
    FPostDefenseOutput PostDefenseProcessing(const FDamageProcessingContext& Context);

    virtual FPreDefenseOutput ModifyDamageProcessing_Implementation(const FDamageProcessingContext& Context);
    virtual FDefenseOutput ApplyDefenseModifiers_Implementation(const FDamageProcessingContext& Context);
    virtual FPostDefenseOutput PostDefenseProcessing_Implementation(const FDamageProcessingContext& Context);
};