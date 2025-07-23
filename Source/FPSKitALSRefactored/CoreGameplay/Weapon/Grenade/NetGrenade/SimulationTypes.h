#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "SimulationTypes.generated.h"
USTRUCT()
struct FPSKITALSREFACTORED_API FNodeLink
{
    GENERATED_BODY()

    int32 NeighborIndex;
    float RestLength;
    float CriticalLength;

    // 🔧 Добавляем недостающие поля
    float Stiffness = 25.0f;
    float InfluenceFactor = 0.4f;
};

USTRUCT()
struct FPSKITALSREFACTORED_API FNode
{
    GENERATED_BODY()

    int32 PositionIndex;
    FVector Velocity;
    bool bFixed;

    TArray<FNodeLink> Links;

    // 🔧 Добавляем недостающее поле
    FVector AccumulatedForce = FVector::ZeroVector;
};

USTRUCT()
struct FPSKITALSREFACTORED_API FInfluenceEntry
{
    GENERATED_BODY()

    int32 Index;
    FVector Velocity;
    float Factor;
};