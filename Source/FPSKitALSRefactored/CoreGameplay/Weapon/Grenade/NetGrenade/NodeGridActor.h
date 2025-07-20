#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "NodeGridActor.generated.h"

USTRUCT()
struct FNodeLink
{
    GENERATED_BODY()

    UPROPERTY()
    int32 NeighborIndex;

    UPROPERTY()
    float RestLength;

    UPROPERTY()
    float CriticalLength;

    UPROPERTY()
    float Stiffness;

    UPROPERTY()
    float InfluenceFactor = 1.0f; // <-- Добавлено
};

USTRUCT()
struct FNode
{
    GENERATED_BODY()

    UPROPERTY()
    FVector Position;

    UPROPERTY()
    FVector PendingPosition;

    UPROPERTY()
    FVector Velocity;

    UPROPERTY()
    FVector AccumulatedForce;

    UPROPERTY()
    bool bFixed = false;

    UPROPERTY()
    TArray<FNodeLink> Links;
};

UCLASS()
class FPSKITALSREFACTORED_API ANodeGridActor : public AActor
{
    GENERATED_BODY()

public:
    ANodeGridActor();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

private:
    void InitializeGrid();
    void PropagateInfluence(int32 SourceIndex, const FVector& SourceVelocity, float InfluenceFactor);
    void OnAsyncTraceResult(const FTraceHandle& Handle, FTraceDatum& Data);

    UPROPERTY()
    TArray<FNode> Nodes;

    //UPROPERTY()
    TMap<FTraceHandle, int32> TraceHandleToNode;
    FTraceDelegate AsyncLineTraceDelegate;

    UPROPERTY(EditAnywhere)
    int32 GridRows = 10;

    UPROPERTY(EditAnywhere)
    int32 GridCols = 10;

    UPROPERTY(EditAnywhere)
    float CellSize = 50.0f;

    UPROPERTY(EditAnywhere)
    float CritLen = 80.0f;

    UPROPERTY(EditAnywhere)
    float Stiffness = 25.0f;

    UPROPERTY(EditAnywhere)
    float InfluenceAttenuation = 0.4f;

    UPROPERTY(EditAnywhere)
    float MinPropagationThreshold = 0.01f;

    UPROPERTY(EditAnywhere)
    FVector GravityDirection = FVector(0, 0, -1);

    UPROPERTY(EditAnywhere)
    float GravityStrength = 980.0f;

    UPROPERTY(EditAnywhere)
    float DampingFactor = 0.96f;

    UPROPERTY(EditAnywhere)
    float OverlapRadius = 2.0f;

    UPROPERTY(EditAnywhere)
    bool bEnableDebugDraw = true;

    UPROPERTY(EditAnywhere)
    float DebugSphereRadius = 4.0f;

    UPROPERTY(EditAnywhere)
    float DebugLineThickness = 0.5f;

    UPROPERTY(EditAnywhere)
    float RCorrect = 1.0f;
};