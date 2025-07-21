#include "NodeGridActor.h"
#include "DrawDebugHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

ANodeGridActor::ANodeGridActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ANodeGridActor::BeginPlay()
{
    Super::BeginPlay();
    InitializeGrid();
}

void ANodeGridActor::InitializeGrid()
{
    Nodes.Empty();

    const int32 TotalCols = GridCols + 1;
    const int32 TotalRows = GridRows + 1;
    const float HalfX = GridCols * 0.5f * CellSize;
    const float HalfY = GridRows * 0.5f * CellSize;

    for (int32 y = 0; y < TotalRows; ++y)
    {
        for (int32 x = 0; x < TotalCols; ++x)
        {
            FVector LocalOffset(x * CellSize - HalfX, y * CellSize - HalfY, 0);
            FVector Pos = GetActorTransform().TransformPosition(LocalOffset);
            Nodes.Add(FNode{ Pos, Pos, FVector::ZeroVector, FVector::ZeroVector, false });
        }
    }

    for (int32 y = 0; y < TotalRows; ++y)
    {
        for (int32 x = 0; x < TotalCols; ++x)
        {
            int32 Index = y * TotalCols + x;

            if (x < TotalCols - 1)
            {
                int32 Right = Index + 1;
                float Rest = (Nodes[Right].Position - Nodes[Index].Position).Size();

                Nodes[Index].Links.Add(FNodeLink{ Right, Rest, CritLen, Stiffness, InfluenceAttenuation });
                Nodes[Right].Links.Add(FNodeLink{ Index, Rest, CritLen, Stiffness, InfluenceAttenuation });
            }

            if (y < TotalRows - 1)
            {
                int32 Down = Index + TotalCols;
                float Rest = (Nodes[Down].Position - Nodes[Index].Position).Size();

                Nodes[Index].Links.Add(FNodeLink{ Down, Rest, CritLen, Stiffness, InfluenceAttenuation });
                Nodes[Down].Links.Add(FNodeLink{ Index, Rest, CritLen, Stiffness, InfluenceAttenuation });
            }
        }
    }
}

void ANodeGridActor::PropagateInfluence(int32 SourceIndex, const FVector& SourceVelocity, float InfluenceFactor)
{
    if (InfluenceFactor < MinPropagationThreshold || !Nodes.IsValidIndex(SourceIndex)) return;

    const FNode& Source = Nodes[SourceIndex];
    if (Source.bFixed) return;

    for (const FNodeLink& Link : Source.Links)
    {
        const int32 NeighborIndex = Link.NeighborIndex;
        if (!Nodes.IsValidIndex(NeighborIndex)) continue;

        FNode& Neighbor = Nodes[NeighborIndex];
        if (Neighbor.bFixed) continue;

        const float Distance = (Neighbor.Position - Source.Position).Size();
        if (Distance > Link.RestLength)
        {
            Neighbor.AccumulatedForce += SourceVelocity * InfluenceFactor;
            PropagateInfluence(NeighborIndex, SourceVelocity, InfluenceFactor * InfluenceAttenuation);
        }
    }
}

void ANodeGridActor::EnforceRigidLinkConstraint(FNode& A, FNode& B, const FNodeLink& Link, float DeltaTime)
{
    FVector Delta = B.Position - A.Position;
    float CurrentLength = Delta.Size();

    if (CurrentLength <= Link.RestLength)
        return; // связь в нормальном состоянии — ни импульса, ни ограничения

    FVector Dir = Delta / CurrentLength;

    if (CurrentLength > Link.CriticalLength)
    {
        // Ограничение длины — не выше критической
        float ClampedLength = FMath::Min(CurrentLength, Link.CriticalLength);
        FVector Target = B.Position - Dir * ClampedLength;
        A.Position = Target;

        // Коррекция скорости — удаляем компонент растяжения
        float RelSpeed = FVector::DotProduct(B.Velocity - A.Velocity, Dir);
        FVector VelocityCorrection = RelSpeed * Dir/* * Link.Stiffness * DeltaTime*/;

        A.Velocity += VelocityCorrection;
        B.Velocity -= VelocityCorrection;
    }
}

void ANodeGridActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    const FVector GravityForce = GravityDirection.GetSafeNormal() * GravityStrength;
    const FCollisionShape ProbeShape = FCollisionShape::MakeSphere(OverlapRadius);
    const FCollisionObjectQueryParams ObjectParams(ECC_WorldStatic | ECC_PhysicsBody | ECC_WorldDynamic);
    const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MeshFixationProbe), false, this);

    StopCount = 0;

    // Распараллеленный расчёт сил и импульсов
    TArray<FVector> LocalForces;
    LocalForces.SetNumZeroed(Nodes.Num());

    TArray<TTuple<int32, FVector, float>> PendingInfluences;
    FCriticalSection InfluenceLock;

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

                FVector Delta = Neighbor.Position - Node.Position;
                float Distance = Delta.Size();
                float Stretch = FMath::Max(0.f, Distance - Link.RestLength);

                UE_LOG(LogTemp, Warning, TEXT("Stretch %f"), Stretch);

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

    // Каскадная передача импульса
    for (const auto& Entry : PendingInfluences)
    {
        int32 Index;
        FVector Velocity;
        float Influence;
        Tie(Index, Velocity, Influence) = Entry;

        PropagateInfluence(Index, Velocity, Influence);
    }

    // Применение сил + фиксация
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        FNode& Node = Nodes[i];

        if (Node.bFixed || Node.Velocity.Length() <= StopVelocity)
            StopCount++;

        if (Node.bFixed) continue;
        Node.AccumulatedForce += LocalForces[i];
    }

    // Движение и фиксация
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        FNode& Node = Nodes[i];
        if (Node.bFixed) continue;

        Node.AccumulatedForce += GravityForce;
        Node.Velocity += Node.AccumulatedForce * DeltaTime;
        Node.PendingPosition = Node.Position + Node.Velocity * DeltaTime;

        bool bFixed = false;

        TArray<FOverlapResult> Overlaps;
        GetWorld()->OverlapMultiByObjectType(
            Overlaps,
            Node.PendingPosition,
            FQuat::Identity,
            ObjectParams,
            ProbeShape,
            QueryParams
        );

        for (const FOverlapResult& Overlap : Overlaps)
        {
            UPrimitiveComponent* HitComp = Overlap.Component.Get();
            if (!HitComp) continue;

            if (HitComp->IsA<UStaticMeshComponent>())
            {
                //Node.Position = Node.PendingPosition;
                Node.Velocity = FVector::ZeroVector;
                Node.bFixed = true;
                bFixed = true;
                break;
            }

            AActor* SourceActor = HitComp->GetOwner();
            if (!SourceActor) continue;

            ACharacter* Char = Cast<ACharacter>(SourceActor);
            if (!Char) continue;

            USkeletalMeshComponent* FixationMesh = Char->GetMesh();
            if (!FixationMesh || !FixationMesh->IsRegistered()) continue;

            TArray<FOverlapResult> MeshOverlaps;
            FCollisionQueryParams MeshParams(TEXT("FixationMeshCheck"), false, this);

            GetWorld()->OverlapMultiByChannel(
                MeshOverlaps,
                Node.PendingPosition,
                FQuat::Identity,
                ECC_Visibility,
                ProbeShape,
                MeshParams
            );

            for (const FOverlapResult& MeshOverlap : MeshOverlaps)
            {
                if (MeshOverlap.Component.Get() == FixationMesh)
                {
                    //Node.Position = Node.PendingPosition;
                    Node.Velocity = FVector::ZeroVector;
                    Node.bFixed = true;
                    bFixed = true;
                    break;
                }
            }

            if (bFixed) break;
        }

        if (!bFixed)
        {
            Node.Position = Node.PendingPosition;
            Node.Velocity *= DampingFactor;
        }

        Node.AccumulatedForce = FVector::ZeroVector;
    }

    // Ограничение растяжения
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        FNode& Node = Nodes[i];
        if (Node.bFixed) continue;

        for (const FNodeLink& Link : Node.Links)
        {
            if (!Nodes.IsValidIndex(Link.NeighborIndex)) continue;
            FNode& Neighbor = Nodes[Link.NeighborIndex];
            if (Neighbor.bFixed) continue;

            EnforceRigidLinkConstraint(Node, Neighbor, Link, DeltaTime);
        }
    }

    // Визуализация
    const float SPart = float(StopCount) / float(Nodes.Num());
    const float DrawLifeTime = SPart >= StopTresholdPart ? 20.f : -1.f;

    if (bEnableDebugDraw)
    {
        const FColor FreeColor = FColor::Green;
        const FColor FixedColor = FColor::Red;

        for (int32 i = 0; i < Nodes.Num(); ++i)
        {
            const FNode& Node = Nodes[i];
            const FColor NodeColor = Node.bFixed ? FixedColor : FreeColor;

            DrawDebugSphere(GetWorld(), Node.Position, DebugSphereRadius, 12, NodeColor, false, DrawLifeTime, 0, 0.5f);

            for (const FNodeLink& Link : Node.Links)
            {
                if (!Nodes.IsValidIndex(Link.NeighborIndex)) continue;
                const FNode& Neighbor = Nodes[Link.NeighborIndex];

                float CurrentLength = (Neighbor.Position - Node.Position).Size();
                float Ratio = FMath::Clamp(CurrentLength / Link.CriticalLength, 0.f, RCorrect);
                FLinearColor LineColor = FLinearColor::LerpUsingHSV(FLinearColor(FreeColor), FLinearColor(FixedColor), Ratio);

                DrawDebugLine(GetWorld(), Node.Position, Neighbor.Position, LineColor.ToFColor(true), false, DrawLifeTime, 0, DebugLineThickness);
            }
        }
    }

    if (StopCount > 0 && SPart >= StopTresholdPart)
    {
        SetActorTickEnabled(false);
        UE_LOG(LogTemp, Log, TEXT("Process stop %d"), StopCount);
    }
}