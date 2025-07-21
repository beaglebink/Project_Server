#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NodeGridActor.generated.h"

USTRUCT(BlueprintType)
struct FNodeLink
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 NeighborIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RestLength;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CriticalLength;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Stiffness;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float InfluenceFactor = 1.0f;
};

USTRUCT(BlueprintType)
struct FNode
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Position;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector PendingPosition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Velocity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector AccumulatedForce;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bFixed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FNodeLink> Links;
};

UCLASS(Blueprintable)
class FPSKITALSREFACTORED_API ANodeGridActor : public AActor
{
    GENERATED_BODY()

public:
    ANodeGridActor();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Доступ к узлам из Blueprint
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FNode> Nodes;

private:
    void InitializeGrid();
    void PropagateInfluence(int32 SourceIndex, const FVector& SourceVelocity, float InfluenceFactor);
    void OnAsyncTraceResult(const FTraceHandle& Handle, FTraceDatum& Data);

    //UPROPERTY() // отключённый map трассировки
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