#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraFunctionLibrary.h"
#include "NodeGridActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnParalysisNPCEvent, ACharacter*, NPC);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRevivalNPCEvent, ACharacter*, User);

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
    int32 PositionIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector PendingPosition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Velocity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector AccumulatedForce;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bFixed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AActor* AttachedActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UPrimitiveComponent* AttachedComponent = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    USkeletalMeshComponent* AttachedMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName AttachedBoneName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FTransform BoneInitialTransform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FTransform NodeLocalTransform;

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

    void PostInitializeComponents();

protected:
    virtual void BeginPlay() override;

    void DestroyThis();

public:
    virtual void Tick(float DeltaTime) override;

private:
    UFUNCTION()
    void DestroyNet(AActor* Reason);
    void InitializeGrid();
    void ApplyForcesParallel();
    void ProcessInfluenceCascade();
    void ApplyMotionAndFixation(float DeltaTime);
    FName FindClosestBoneToPoint(USkeletalMeshComponent* SkeletalMesh, const FVector& Point) const;
    void ParalyzeCharacter(ACharacter* Char);
    void ApplyRigidConstraints(float DeltaTime);
    void DrawDebugState();
    void IterateUniqueLinks();
    void PropagateInfluence(int32 SourceIndex, const FVector& SourceVelocity, float InfluenceFactor);
    void EnforceRigidLinkConstraint(FNode& A, FNode& B, const FNodeLink& Link, float DeltaTime);

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FNode> Nodes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FVector> NodePositions;

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
    float OverlapRadius = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    float StopVelocity = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    float StopTresholdPart = 0.95f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
	float DesrtoyTime = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = "true"))
    bool bEnableDebugDraw = true;

    UPROPERTY(EditAnywhere)
    float DebugSphereRadius = 0.5f;

    UPROPERTY(EditAnywhere)
    float DebugLineThickness = 0.5f;

    UPROPERTY(EditAnywhere)
    float RCorrect = 1.0f;

    UPROPERTY(BlueprintAssignable)
	FOnParalysisNPCEvent OnParalysisNPC;

	UPROPERTY(BlueprintAssignable)
	FOnRevivalNPCEvent OnRevivalNPC;

protected:
    UPROPERTY()
    UNiagaraComponent* NiagaraComp;

    UPROPERTY(EditAnywhere)
    UNiagaraSystem* NiagaraSystemAsset;

    UPROPERTY()
	TArray< UNiagaraComponent*> NiagaraComponents;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> RibbonStartIndices;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<int32> RibbonEndIndices;

private:
    int32 StopCount = 0;

    TArray<TTuple<int32, FVector, float>> PendingInfluences;

    FTimerHandle StunTimerHandle;

    TSet<TPair<int32, int32>> UniqueLinks;

    UPROPERTY()
    TArray<FVector> Ribbons;
};