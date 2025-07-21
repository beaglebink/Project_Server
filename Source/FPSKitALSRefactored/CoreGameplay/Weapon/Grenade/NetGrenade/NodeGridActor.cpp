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
        if (Distance > Link.CriticalLength)
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
        // 🔒 Ограничение длины — не выше критической
        float ClampedLength = FMath::Min(CurrentLength, Link.CriticalLength);
        FVector Target = B.Position - Dir * ClampedLength;
        A.Position = Target;

        // 🔧 Коррекция скорости — удаляем компонент растяжения
        float RelSpeed = FVector::DotProduct(B.Velocity - A.Velocity, Dir);
        FVector VelocityCorrection = RelSpeed * Dir/* * Link.Stiffness * DeltaTime*/;

        A.Velocity += VelocityCorrection;
        B.Velocity -= VelocityCorrection;
    }
    /*
    // 🧲 Сжатие обратно к RestLength, если связь остаётся растянутой
    if (CurrentLength > Link.RestLength)
    {
        float Compression = CurrentLength - Link.RestLength;
        FVector RestoringForce = -Dir * Compression * Link.Stiffness;

        A.AccumulatedForce += RestoringForce;
        if (!B.bFixed)
            B.AccumulatedForce -= RestoringForce;
    }
    */
}

void ANodeGridActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    const FVector GravityForce = GravityDirection.GetSafeNormal() * GravityStrength;
    const FCollisionShape ProbeShape = FCollisionShape::MakeSphere(OverlapRadius);
    const FCollisionObjectQueryParams ObjectParams(ECC_WorldStatic | ECC_PhysicsBody | ECC_WorldDynamic);
    const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MeshFixationProbe), false, this);

	StopCount = 0; // Сброс счётчика остановленных узлов

    // ⬇️ Фаза накопления сил
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        FNode& Node = Nodes[i];


        UE_LOG(LogTemp, Log, TEXT("[Node %d] Fixed: %s | Vel: %s (%f) | Pos: %s"),
            i,
            Node.bFixed ? TEXT("true") : TEXT("false"),
            *Node.Velocity.ToString(),
            Node.Velocity.Length(),
            *Node.Position.ToString());

		if (Node.bFixed && Node.Velocity.Length() <= StopVelocity)
		{
			StopCount++; // Увеличиваем счётчик остановленных узлов
		}

        if (Node.bFixed) continue;

        for (const FNodeLink& Link : Node.Links)
        {
            if (!Nodes.IsValidIndex(Link.NeighborIndex)) continue;
            FNode& Neighbor = Nodes[Link.NeighborIndex];
            //if (Neighbor.bFixed) continue;

            FVector Delta = Neighbor.Position - Node.Position;
            float Distance = Delta.Size();
            float Stretch = FMath::Max(0.f, Distance - Link.RestLength);

            if (Stretch > 0.f)
            {
                FVector Dir = Delta / Distance;
                FVector Force = Dir * Stretch * Link.Stiffness;
                Node.AccumulatedForce += Force;
                Neighbor.AccumulatedForce -= Force;
            }

            if (Distance > Link.RestLength)
            {
                Node.AccumulatedForce += Delta * Link.InfluenceFactor;
                PropagateInfluence(Link.NeighborIndex, Neighbor.Velocity, Link.InfluenceFactor);
            }
        }
    }

    // ⬇️ Фаза движения и фиксации
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        FNode& Node = Nodes[i];

        UE_LOG(LogTemp, Log, TEXT("[Node %d] Accumulated Force: %s"),
            i,
            *Node.AccumulatedForce.ToString());

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
                Node.Position = Node.PendingPosition;
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
                    Node.Position = Node.PendingPosition;
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

    // ⬇️ Архитектурное ограничение растяжения связей
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

    // ⬇️ Визуализация
    if (bEnableDebugDraw)
    {
        const FColor FreeColor = FColor::Green;
        const FColor FixedColor = FColor::Red;

        for (int32 i = 0; i < Nodes.Num(); ++i)
        {
            const FNode& Node = Nodes[i];
            const FColor NodeColor = Node.bFixed ? FixedColor : FreeColor;

            DrawDebugSphere(GetWorld(), Node.Position, DebugSphereRadius, 12, NodeColor, false, -1.f, 0, 0.5f);

            for (const FNodeLink& Link : Node.Links)
            {
                if (!Nodes.IsValidIndex(Link.NeighborIndex)) continue;
                const FNode& Neighbor = Nodes[Link.NeighborIndex];

                float CurrentLength = (Neighbor.Position - Node.Position).Size();
                float Ratio = FMath::Clamp(CurrentLength / Link.CriticalLength, 0.f, RCorrect);
                FLinearColor LineColor = FLinearColor::LerpUsingHSV(FLinearColor(FreeColor), FLinearColor(FixedColor), Ratio);

                DrawDebugLine(GetWorld(), Node.Position, Neighbor.Position, LineColor.ToFColor(true), false, -1.f, 0, DebugLineThickness);
            }
        }
    }
}