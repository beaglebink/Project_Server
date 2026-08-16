#include "DamageProcessingEffect.h"

FPreDefenseOutput UDamageProcessingEffect::ModifyDamageProcessing_Implementation(const FDamageProcessingContext& Context)
{
    FPreDefenseOutput Out;
    Out.NewIncomingDamage = Context.IncomingDamage;
    return Out;
}

FDefenseOutput UDamageProcessingEffect::ApplyDefenseModifiers_Implementation(const FDamageProcessingContext& Context)
{
    FDefenseOutput Out;
    Out.NewModifiers.ResistanceMultipliers = Context.ResistanceMultipliers;
    Out.NewModifiers.ReserveDamageMultipliers = Context.ReserveDamageMultipliers;
    Out.NewModifiers.IgnoreLayer = Context.IgnoreLayer;
    return Out;
}

FPostDefenseOutput UDamageProcessingEffect::PostDefenseProcessing_Implementation(const FDamageProcessingContext& Context)
{
    FPostDefenseOutput Out;
    Out.NewFinalHealthDamage = Context.FinalHealthDamage;
    Out.NewStaggerChanceModifier = Context.StaggerChanceModifier;
    // Поле bNewForceStagger остаётся без изменений, его устанавливает блюпринт
    Out.bNewForceStagger = false; // по умолчанию, но блюпринт переопределит
    Out.bNewForceStaggerCooldown = Context.ForceStaggerCooldown; // передаём текущий кулдаун
    return Out;
}