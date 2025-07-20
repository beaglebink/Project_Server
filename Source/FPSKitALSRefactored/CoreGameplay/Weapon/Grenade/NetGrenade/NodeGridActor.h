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
    float RestLength = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Stiffness = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CriticalLength = 120.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float InfluenceFactor = 0.2f;
};

USTRUCT(BlueprintType)
struct FNode
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Position;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Velocity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bFixed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector AccumulatedForce = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FNodeLink> Links;
};

UCLASS()
class FPSKITALSREFACTORED_API ANodeGridActor : public AActor
{
    GENERATED_BODY()

public:
    ANodeGridActor();

    UPROPERTY(EditAnywhere)
    int32 GridRows = 10;

    UPROPERTY(EditAnywhere)
    int32 GridCols = 10;

    UPROPERTY(EditAnywhere)
    float CellSize = 100.f;

    UPROPERTY(EditAnywhere)
    float DampingFactor = 0.98f;

    UPROPERTY(EditAnywhere)
    FVector GravityDirection = FVector(0, 0, -1);

    UPROPERTY(EditAnywhere)
    float GravityStrength = 980.f;

    UPROPERTY(EditAnywhere)
    float InfluenceAttenuation = 0.5f;

    UPROPERTY(EditAnywhere)
    float MinPropagationThreshold = 0.005f;

    UPROPERTY(EditAnywhere)
    float CritLen = 150.f;

    UPROPERTY(EditAnywhere)
    float Stiffness = 10.f;

    UPROPERTY(EditAnywhere)
    float DebugSphereRadius = 2.f;

    UPROPERTY(EditAnywhere)
    float DebugLineThickness = 1.f;

    UPROPERTY(EditAnywhere)
    float RCorrect = 1.5f;

    UPROPERTY(EditAnywhere)
    bool bEnableDebugDraw = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FNode> Nodes;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    void InitializeGrid();
    void PropagateInfluence(int32 SourceIndex, const FVector& SourceVelocity, float InfluenceFactor);
};