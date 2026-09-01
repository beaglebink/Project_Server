#include "EnemyDamageConfig.h"
#include "Misc/DataValidation.h"

#if WITH_EDITOR
void UEnemyDamageConfig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    ValidateHitZones();
    ValidateHealthZones();
}
#endif

bool UEnemyDamageConfig::ValidateHitZones() const
{
    TSet<FName> UsedBones;
    TSet<FName> UsedComponents;
    bool bValid = true;

    for (const FHitZoneDefinition& Zone : HitZones)
    {
        for (const FName& Bone : Zone.BoneNames)
        {
            if (UsedBones.Contains(Bone))
            {
                UE_LOG(LogTemp, Error, TEXT("Bone %s used in multiple hit zones in %s"), *Bone.ToString(), *GetNameSafe(this));
                bValid = false;
            }
            UsedBones.Add(Bone);
        }

        for (const FName& Comp : Zone.ComponentNames)
        {
            if (UsedComponents.Contains(Comp))
            {
                UE_LOG(LogTemp, Error, TEXT("Component %s used in multiple hit zones in %s"), *Comp.ToString(), *GetNameSafe(this));
                bValid = false;
            }
            UsedComponents.Add(Comp);
        }
    }
    return bValid;
}

bool UEnemyDamageConfig::ValidateHealthZones() const
{
    if (HealthZones.Num() == 0)
        return true;

    // Sort in descending order of UpperBound
    // Сортировка по убыванию UpperBound
    TArray<FHealthZoneDefinition> Sorted = HealthZones;
    Sorted.Sort([](const FHealthZoneDefinition& A, const FHealthZoneDefinition& B) {
        return A.UpperBound > B.UpperBound;
        });

    // Check that the first zone covers MaxHealth
    // Проверка, что первая зона охватывает MaxHealth
    if (Sorted[0].UpperBound != MaxHealth)
    {
        UE_LOG(LogTemp, Error, TEXT("First health zone UpperBound must match MaxHealth in %s"), *GetNameSafe(this));
        return false;
    }
    // Check that the last zone has LowerBound = 0
    // Проверка, что последняя зона имеет LowerBound = 0
    if (Sorted.Last().LowerBound != 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Last health zone LowerBound must be 0 in %s"), *GetNameSafe(this));
        return false;
    }
    // Check continuity
    // Проверка непрерывности
    for (int32 i = 0; i < Sorted.Num() - 1; ++i)
    {
        if (Sorted[i].LowerBound != Sorted[i + 1].UpperBound)
        {
            UE_LOG(LogTemp, Error, TEXT("Gap or overlap between health zones %s and %s in %s"),
                *Sorted[i].ZoneTag.ToString(), *Sorted[i + 1].ZoneTag.ToString(), *GetNameSafe(this));
            return false;
        }
    }
    return true;
}