#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SimulationTypes.h"
#include "NiagaraFunctionLibrary.h"
#include "NodeGridActor.generated.h"

// 🔍 Forward declarations
class URibbonLinkAdapter;
class UNiagaraComponent;
class UNiagaraSystem;
class UNiagaraDataInterface_RibbonLinks;
class UNiagaraDataInterfaceArrayVector;

UCLASS()
class FPSKITALSREFACTORED_API ANodeGridActor : public AActor
{
    GENERATED_BODY()

public:
    ANodeGridActor();

protected:
    virtual void BeginPlay() override;
    virtual void PostInitializeComponents() override;
    virtual void Tick(float DeltaTime) override;

    void InitializeGrid();
    void ApplyForcesParallel();
    void ProcessInfluenceCascade();
    void ApplyMotionAndFixation(float DeltaTime);
    void ApplyRigidConstraints(float DeltaTime);
    void DrawDebugState();

    // 🧠 Симуляционные данные
    UPROPERTY()
    TArray<FNode> Nodes;

    UPROPERTY()
    TArray<FVector> NodePositions;

    UPROPERTY()
    TArray<FInfluenceEntry> PendingInfluences;

    UPROPERTY()
    int32 StopCount = 0;

    // 🔁 Niagara визуализация
    UPROPERTY()
    UNiagaraComponent* NiagaraComp;

    UPROPERTY(EditAnywhere)
    UNiagaraSystem* NiagaraSystemAsset;

    UPROPERTY()
    UNiagaraDataInterface_RibbonLinks* RibbonNDI;

    UPROPERTY()
    URibbonLinkAdapter* RibbonLinkAdapter;

    UPROPERTY(EditAnywhere)
    UNiagaraParameterCollection* NiagaraParamCollection;

public:
    // 📦 Параметры сетки и симуляции
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
};