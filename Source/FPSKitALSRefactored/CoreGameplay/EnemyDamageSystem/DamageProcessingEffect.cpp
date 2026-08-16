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
    //Out.NewModifiers.BypassReserve = Context.BypassReserve;
    Out.NewModifiers.IgnoreLayer = Context.IgnoreLayer;
    return Out;
}

FPostDefenseOutput UDamageProcessingEffect::PostDefenseProcessing_Implementation(const FDamageProcessingContext& Context)
{
    FPostDefenseOutput Out;
    Out.NewFinalHealthDamage = Context.FinalHealthDamage;
    Out.NewStaggerChanceModifier = Context.StaggerChanceModifier;
    //Out.bNewForceStagger = Context.bForceStagger;
    return Out;
}