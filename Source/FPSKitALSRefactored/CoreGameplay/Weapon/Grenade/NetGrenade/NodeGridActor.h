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

struct FInfluenceEntry
{
    int32 Index;
    FVector Velocity;
    float Factor;
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

private:
    void InitializeGrid();
    void PropagateInfluence(int32 SourceIndex, const FVector& SourceVelocity, float InfluenceFactor);
    void EnforceRigidLinkConstraint(FNode& A, FNode& B, const FNodeLink& Link, float DeltaTime);

    // 🔁 Вынесенные фазы
    void ApplyForcesParallel();
    void ProcessInfluenceCascade();
    void ApplyMotionAndFixation(float DeltaTime);
    void ApplyRigidConstraints(float DeltaTime);
    void DrawDebugState();

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FNode> Nodes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    int32 GridRows = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    int32 GridCols = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    float CellSize = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    float CritLen = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    float Vel = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    float Stiffness = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    float InfluenceAttenuation = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    float MinPropagationThreshold = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    FVector GravityDirection = FVector(0, 0, -1);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    float GravityStrength = 980.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    float DampingFactor = 0.96f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    float OverlapRadius = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    float StopVelocity = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    float StopTresholdPart = 0.95f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    bool bEnableDebugDraw = true;

    UPROPERTY(EditAnywhere)
    float DebugSphereRadius = 0.5f;

    UPROPERTY(EditAnywhere)
    float DebugLineThickness = 0.5f;

    UPROPERTY(EditAnywhere)
    float RCorrect = 1.0f;

private:
    int32 StopCount = 0;

    // 🔁 Каскадная очередь импульсов
    TArray<TTuple<int32, FVector, float>> PendingInfluences;
};