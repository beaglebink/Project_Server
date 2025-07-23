#include "NodeGridActor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraDataInterface_RibbonLinks.h"
#include "RibbonLinkAdapter.h"
#include "DrawDebugHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include <Kismet/KismetMathLibrary.h>

ANodeGridActor::ANodeGridActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ANodeGridActor::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    NiagaraComp = NewObject<UNiagaraComponent>(this, TEXT("NodeNiagara"));
    NiagaraComp->SetupAttachment(RootComponent);
    NiagaraComp->RegisterComponent();
    NiagaraComp->SetAsset(NiagaraSystemAsset);

    // 🔍 Получаем NDI из Niagara параметров
    FNiagaraVariable Var(FNiagaraTypeDefinition(UNiagaraDataInterface_RibbonLinks::StaticClass()), TEXT("RibbonNDI"));
    UNiagaraDataInterface* DI = NiagaraComp->GetOverrideParameters().GetDataInterface(Var);
    RibbonNDI = Cast<UNiagaraDataInterface_RibbonLinks>(DI);

    RibbonLinkAdapter = NewObject<URibbonLinkAdapter>(this);
}

void ANodeGridActor::ApplyForcesParallel()
{
    TArray<FVector> LocalForces;
    LocalForces.SetNumZeroed(Nodes.Num());

    FCriticalSection InfluenceLock;
    PendingInfluences.Reset();

    ParallelFor(Nodes.Num(), [&](int32 i)
        {
            const FNode& Node = Nodes[i];
            if (Node.bFixed) return;

            FVector AccForce = FVector::ZeroVector;

            for (const FNodeLink& Link : Node.Links)
            {
                int32 NeighborIndex = Link.NeighborIndex;
                if (!Nodes.IsValidIndex(NeighborIndex)) continue;
                const FNode& Neighbor = Nodes[NeighborIndex];

                FVector Delta = NodePositions[Neighbor.PositionIndex] - NodePositions[Node.PositionIndex];
                float Distance = Delta.Size();
                float Stretch = FMath::Max(0.f, Distance - Link.RestLength);

                if (Stretch > 0.f)
                {
                    FVector Dir = Delta / Distance;
                    FVector Force = Dir * Stretch * Link.Stiffness;
                    AccForce += Force;
                    LocalForces[NeighborIndex] -= Force;
                }

                if (Distance > Link.RestLength)
                {
                    float Ratio = Distance / Link.RestLength;
                    float CriticalRatio = FMath::Max(Link.CriticalLength / Link.RestLength, 1.1f);

                    float InfluenceScale = FMath::GetMappedRangeValueClamped(
                        TRange<float>(1.1f, CriticalRatio),
                        TRange<float>(0.1f, 1.0f),
                        Ratio
                    );

                    FVector Dir = Delta / Distance;
                    FVector Impulse = Dir * Link.InfluenceFactor * InfluenceScale;

                    LocalForces[NeighborIndex] += Impulse;
                    LocalForces[i] -= Impulse;

                    InfluenceLock.Lock();
                    PendingInfluences.Add({ NeighborIndex, Impulse, Link.InfluenceFactor * InfluenceScale });
                    InfluenceLock.Unlock();
                }
            }

            LocalForces[i] += AccForce;
        });

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        FNode& Node = Nodes[i];

        if (Node.bFixed || Node.Velocity.Length() <= StopVelocity)
            StopCount++;

        if (Node.bFixed) continue;
        Node.AccumulatedForce += LocalForces[i];
    }
}

void ANodeGridActor::ProcessInfluenceCascade()
{
    TQueue<FInfluenceEntry> InfluenceQueue;

    for (const auto& Entry : PendingInfluences)
    {
        int32 Index = Entry.Index;
        FVector Velocity = Entry.Velocity;
        float Factor = Entry.Factor;

        InfluenceQueue.Enqueue({ Index, Velocity, Factor });
    }

    TSet<TPair<int32, int32>> VisitedLinks;

    while (!InfluenceQueue.IsEmpty())
    {
        FInfluenceEntry Current;
        InfluenceQueue.Dequeue(Current);

        if (Current.Factor < MinPropagationThreshold || !Nodes.IsValidIndex(Current.Index)) continue;

        const FNode& Source = Nodes[Current.Index];
        if (Source.bFixed) continue;

        for (const FNodeLink& Link : Source.Links)
        {
            int32 NeighborIndex = Link.NeighborIndex;
            if (!Nodes.IsValidIndex(NeighborIndex)) continue;

            FNode& Neighbor = Nodes[NeighborIndex];
            if (Neighbor.bFixed) continue;

            FVector Delta = NodePositions[Neighbor.PositionIndex] - NodePositions[Source.PositionIndex];
            float Distance = Delta.Size();

            if (Distance > Link.RestLength)
            {
                float Ratio = Distance / Link.RestLength;
                float CriticalRatio = Link.CriticalLength / Link.RestLength;

                float InfluenceScale = (Ratio < CriticalRatio)
                    ? FMath::GetMappedRangeValueClamped(
                        TRange<float>(1.0f, CriticalRatio),
                        TRange<float>(0.1f, 1.0f),
                        Ratio)
                    : 2.0f;

                FVector Impulse = Current.Velocity * Current.Factor * InfluenceScale;
                Neighbor.AccumulatedForce += Impulse;

                int32 A = FMath::Min(Current.Index, NeighborIndex);
                int32 B = FMath::Max(Current.Index, NeighborIndex);
                TPair<int32, int32> LinkKey(A, B);

                if (!VisitedLinks.Contains(LinkKey))
                {
                    VisitedLinks.Add(LinkKey);
                    InfluenceQueue.Enqueue({ NeighborIndex, Current.Velocity, Current.Factor * InfluenceScale * InfluenceAttenuation });
                }
            }
        }
    }
}

void ANodeGridActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    StopCount = 0;
    PendingInfluences.Reset();

    ApplyForcesParallel();
    ProcessInfluenceCascade();
    ApplyMotionAndFixation(DeltaTime);
    ApplyRigidConstraints(DeltaTime);
    DrawDebugState();

    // 🔁 Передача координат в Niagara через Data Interface
    UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
        NiagaraComp,
        FName("NodePositions"),
        NodePositions
    );

    if (RibbonLinkAdapter && RibbonNDI)
    {
        RibbonLinkAdapter->UpdateRibbonLinks(Nodes, RibbonNDI);
    }

    const float SPart = float(StopCount) / float(Nodes.Num());
    if (StopCount > 0 && SPart >= StopTresholdPart)
    {
        SetActorTickEnabled(false);
        UE_LOG(LogTemp, Log, TEXT("Process stop %d"), StopCount);
    }
}

void ANodeGridActor::ApplyMotionAndFixation(float DeltaTime)
{
    const FVector GravityForce = GravityDirection.GetSafeNormal() * GravityStrength;

    TArray<FVector> PendingPositions;
    TArray<FVector> PendingVelocities;
    TArray<bool> FixationFlags;

    PendingPositions.SetNumUninitialized(Nodes.Num());
    PendingVelocities.SetNumUninitialized(Nodes.Num());
    FixationFlags.SetNumZeroed(Nodes.Num());

    ParallelFor(Nodes.Num(), [&](int32 i)
        {
            const FNode& Node = Nodes[i];
            if (Node.bFixed) return;

            FVector Velocity = Node.Velocity + (Node.AccumulatedForce + GravityForce) * DeltaTime;
            FVector Position = NodePositions[Node.PositionIndex] + Velocity * DeltaTime;

            PendingPositions[i] = Position;
            PendingVelocities[i] = Velocity * DampingFactor;
        });

    const FCollisionShape ProbeShape = FCollisionShape::MakeSphere(OverlapRadius);
    const FCollisionObjectQueryParams ObjectParams(ECC_WorldStatic | ECC_PhysicsBody | ECC_WorldDynamic);
    const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MeshFixationProbe), false, this);

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        FNode& Node = Nodes[i];
        if (Node.bFixed) continue;

        const FVector& TestPosition = PendingPositions[i];

        TArray<FOverlapResult> Overlaps;
        GetWorld()->OverlapMultiByObjectType(
            Overlaps, TestPosition, FQuat::Identity, ObjectParams, ProbeShape, QueryParams
        );

        for (const FOverlapResult& Overlap : Overlaps)
        {
            UPrimitiveComponent* HitComp = Overlap.Component.Get();
            if (!HitComp) continue;

            if (HitComp->IsA<UStaticMeshComponent>())
            {
                FixationFlags[i] = true;
                break;
            }

            ACharacter* Char = Cast<ACharacter>(HitComp->GetOwner());
            if (!Char) continue;

            USkeletalMeshComponent* FixationMesh = Char->GetMesh();
            if (!FixationMesh || !FixationMesh->IsRegistered()) continue;

            TArray<FOverlapResult> MeshOverlaps;
            FCollisionQueryParams MeshParams(TEXT("FixationMeshCheck"), false, this);

            GetWorld()->OverlapMultiByChannel(
                MeshOverlaps, TestPosition, FQuat::Identity,
                ECC_Visibility, ProbeShape, MeshParams
            );

            for (const FOverlapResult& MeshOverlap : MeshOverlaps)
            {
                if (MeshOverlap.Component.Get() == FixationMesh)
                {
                    FixationFlags[i] = true;
                    break;
                }
            }

            if (FixationFlags[i]) break;
        }
    }

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        FNode& Node = Nodes[i];
        if (Node.bFixed) continue;

        if (FixationFlags[i])
        {
            NodePositions[Node.PositionIndex] = (PendingPositions[i] + NodePositions[Node.PositionIndex]) / 2;
            Node.Velocity = FVector::ZeroVector;
            Node.bFixed = true;
        }
        else
        {
            NodePositions[Node.PositionIndex] = PendingPositions[i];
            Node.Velocity = PendingVelocities[i];
        }

        Node.AccumulatedForce = FVector::ZeroVector;
    }
}

void ANodeGridActor::ApplyRigidConstraints(float DeltaTime)
{
    TSet<TPair<int32, int32>> ProcessedLinks;

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        FNode& A = Nodes[i];
        if (A.bFixed) continue;

        for (const FNodeLink& Link : A.Links)
        {
            int32 j = Link.NeighborIndex;
            if (!Nodes.IsValidIndex(j)) continue;

            FNode& B = Nodes[j];
            if (B.bFixed) continue;

            TPair<int32, int32> Key = (i < j) ? TPair<int32, int32>(i, j) : TPair<int32, int32>(j, i);
            if (ProcessedLinks.Contains(Key)) continue;
            ProcessedLinks.Add(Key);

            FVector Delta = NodePositions[B.PositionIndex] - NodePositions[A.PositionIndex];
            float CurrentLength = Delta.Size();
            if (CurrentLength <= Link.RestLength) continue;

            FVector Dir = Delta / CurrentLength;

            if (CurrentLength > Link.CriticalLength)
            {
                float ClampedLength = FMath::Min(CurrentLength, Link.CriticalLength);
                FVector MidPoint = (NodePositions[A.PositionIndex] + NodePositions[B.PositionIndex]) * 0.5f;
                FVector TargetA = MidPoint - Dir * ClampedLength * 0.5f;
                FVector TargetB = MidPoint + Dir * ClampedLength * 0.5f;

                NodePositions[A.PositionIndex] = TargetA;
                NodePositions[B.PositionIndex] = TargetB;

                FVector RelVel = B.Velocity - A.Velocity;
                float RelSpeed = FVector::DotProduct(RelVel, Dir);
                FVector VelCorrection = RelSpeed * Dir;

                A.Velocity += VelCorrection;
                B.Velocity -= VelCorrection;
            }
        }
    }
}

void ANodeGridActor::DrawDebugState()
{
    const float SPart = float(StopCount) / float(Nodes.Num());
    const float DrawLifeTime = SPart >= StopTresholdPart ? 20.f : -1.f;

    if (!bEnableDebugDraw) return;

    const FColor FreeColor = FColor::Green;
    const FColor FixedColor = FColor::Red;

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        const FNode& Node = Nodes[i];
        const FColor NodeColor = Node.bFixed ? FixedColor : FreeColor;
        const FVector& Pos = NodePositions[Node.PositionIndex];

        DrawDebugSphere(GetWorld(), Pos, DebugSphereRadius, 12, NodeColor, false, DrawLifeTime, 0, 0.5f);

        for (const FNodeLink& Link : Node.Links)
        {
            if (!Nodes.IsValidIndex(Link.NeighborIndex)) continue;
            const FNode& Neighbor = Nodes[Link.NeighborIndex];

            float CurrentLength = (NodePositions[Neighbor.PositionIndex] - Pos).Size();
            float Ratio = FMath::Clamp(CurrentLength / Link.CriticalLength, 0.f, RCorrect);
            FLinearColor LineColor = FLinearColor::LerpUsingHSV(FLinearColor(FreeColor), FLinearColor(FixedColor), Ratio);

            DrawDebugLine(GetWorld(), Pos, NodePositions[Neighbor.PositionIndex], LineColor.ToFColor(true), false, DrawLifeTime, 0, DebugLineThickness);
        }
    }
}

void ANodeGridActor::BeginPlay()
{
    Super::BeginPlay();
    InitializeGrid();
}

void ANodeGridActor::InitializeGrid()
{
    Nodes.Reset();
    NodePositions.Reset();

    const int32 Total = GridRows * GridCols;
    Nodes.SetNum(Total);
    NodePositions.SetNum(Total);

    const float HalfX = GridCols * 0.5f * CellSize;
    const float HalfY = GridRows * 0.5f * CellSize;

    for (int32 Row = 0; Row < GridRows; ++Row)
    {
        for (int32 Col = 0; Col < GridCols; ++Col)
        {
            const int32 Index = Row * GridCols + Col;
			FVector LocalOffset = FVector(Col * CellSize - HalfX, Row * CellSize - HalfY, 0);
            FVector Position = GetActorTransform().TransformPosition(LocalOffset);//GetActorLocation() + FVector(Row * CellSize, Col * CellSize, 0);
            FVector W_Velocity = UKismetMathLibrary::TransformDirection(GetActorTransform(), FVector(0.f, 0.f, Vel));

            NodePositions[Index] = Position;

            FNode& Node = Nodes[Index];
            Node.PositionIndex = Index;
            Node.Velocity = W_Velocity;
            Node.bFixed = false;
            Node.AccumulatedForce = FVector::ZeroVector;

            // Связи по сетке
            auto TryAddLink = [&](int32 NeighborIndex)
                {
                    if (!Nodes.IsValidIndex(NeighborIndex)) return;
                    FNodeLink Link;
                    Link.NeighborIndex = NeighborIndex;
                    Link.RestLength = CellSize;
                    Link.CriticalLength = CritLen;
                    Node.Links.Add(Link);
                };

            if (Row > 0) TryAddLink((Row - 1) * GridCols + Col);     // вверх
            if (Row < GridRows - 1) TryAddLink((Row + 1) * GridCols + Col); // вниз
            if (Col > 0) TryAddLink(Row * GridCols + (Col - 1));     // влево
            if (Col < GridCols - 1) TryAddLink(Row * GridCols + (Col + 1)); // вправо
        }
    }
}