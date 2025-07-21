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

    // Создание узлов
    for (int32 y = 0; y <= GridRows; ++y)
    {
        for (int32 x = 0; x <= GridCols; ++x)
        {
            FVector LocalOffset(x * CellSize - HalfX, y * CellSize - HalfY, 0);
            FVector Pos = GetActorTransform().TransformPosition(LocalOffset);
            Nodes.Add(FNode{ Pos, Pos, FVector::ZeroVector, FVector::ZeroVector, false });
        }
    }

    // 🔹 Построение связей по узлам (вправо и вниз)
    for (int32 y = 0; y < TotalRows; ++y)
    {
        for (int32 x = 0; x < TotalCols; ++x)
        {
            int32 Index = y * TotalCols + x;

            // Связь вправо
            if (x < TotalCols - 1)
            {
                int32 Right = Index + 1;
                float Rest = (Nodes[Right].Position - Nodes[Index].Position).Size();

                Nodes[Index].Links.Add(FNodeLink{ Right, Rest, CritLen, Stiffness, InfluenceAttenuation });
                Nodes[Right].Links.Add(FNodeLink{ Index, Rest, CritLen, Stiffness, InfluenceAttenuation });
            }

            // Связь вниз
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
			Neighbor.AccumulatedForce += SourceVelocity * InfluenceFactor; // Проверить знак! Возможно, нужно инвертировать вектор
            PropagateInfluence(NeighborIndex, SourceVelocity, InfluenceFactor * InfluenceAttenuation);
        }
    }
}
 
void ANodeGridActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

        const FVector GravityForce = GravityDirection.GetSafeNormal() * GravityStrength;
    const FCollisionShape ProbeShape = FCollisionShape::MakeSphere(OverlapRadius);
    FCollisionObjectQueryParams ObjectParams(ECC_WorldStatic | ECC_PhysicsBody | ECC_WorldDynamic);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MeshFixationProbe), false, this);

    // Фаза накопления сил
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        FNode& Node = Nodes[i];

        UE_LOG(LogTemp, Log, TEXT("[Node %d] Fixed: %s | Vel: %s (%f) | Pos: %s"),
            i,
            Node.bFixed ? TEXT("true") : TEXT("false"),
            *Node.Velocity.ToString(),
            Node.Velocity.Length(),
            *Node.Position.ToString());

        if (Node.bFixed) continue;

        for (const FNodeLink& Link : Node.Links)
        {
            if (!Nodes.IsValidIndex(Link.NeighborIndex)) continue;
            FNode& Neighbor = Nodes[Link.NeighborIndex];

            FVector Delta = Neighbor.Position - Node.Position;
            float Distance = Delta.Size();
            float Stretch = FMath::Max(0.f, Distance - Link.RestLength);

            if (Stretch > 0.f)
            {
                FVector Dir = Delta / Distance;

                float OverStretchRatio = FMath::Max(Distance / Link.RestLength, 1.0f);
                float StretchMultiplier = FMath::Pow(OverStretchRatio, 2.0f);


                FVector Force = Dir * Stretch * Link.Stiffness;
                //FVector Force = Dir * Stretch * Link.Stiffness * StretchMultiplier;


                Node.AccumulatedForce += Force;
                if (!Neighbor.bFixed)
                    Neighbor.AccumulatedForce -= Force;
            }

            if (Distance > Link.CriticalLength && !Neighbor.bFixed)
            {
                Node.AccumulatedForce += Delta * Link.InfluenceFactor; // Проверить знак!
                PropagateInfluence(Link.NeighborIndex, Neighbor.Velocity, Link.InfluenceFactor);
            }
        }
    }

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        FNode& Node = Nodes[i];
        if (Node.bFixed) continue;

        // Движение
        Node.AccumulatedForce += GravityForce;

		UE_LOG(LogTemp, Log, TEXT("[Node %d] Accumulated Force: %s"),
			i,
			*Node.AccumulatedForce.ToString());

        Node.Velocity += Node.AccumulatedForce * DeltaTime;
        Node.PendingPosition = Node.Position + Node.Velocity * DeltaTime;

        bool bFixed = false;

        // Поиск пересекающихся компонентов
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

            // StaticMesh: реагируем сразу
            if (HitComp->IsA<UStaticMeshComponent>())
            {
                Node.Position = Node.PendingPosition;
                Node.Velocity = FVector::ZeroVector;
                Node.bFixed = true;
                bFixed = true;
                break;
            }

            // SkeletalMesh: фильтрация через SourceActor
            AActor* SourceActor = HitComp->GetOwner();
            if (!SourceActor) continue;

            ACharacter* Char = Cast<ACharacter>(SourceActor);
            if (!Char) continue;

            USkeletalMeshComponent* FixationMesh = Char->GetMesh();
            if (!FixationMesh || !FixationMesh->IsRegistered()) continue;

            // Явная проверка: меш должен быть в OverlapMultiByChannel
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

        // Движение при отсутствии фиксации
        if (!bFixed)
        {
            Node.Position = Node.PendingPosition;
            Node.Velocity *= DampingFactor;
        }

        Node.AccumulatedForce = FVector::ZeroVector;
    }

    // Отладочная визуализация
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

                const float CurrentLength = (Neighbor.Position - Node.Position).Size();
                const float Ratio = FMath::Clamp(CurrentLength / Link.CriticalLength, 0.f, RCorrect);
                const FLinearColor LineColor = FLinearColor::LerpUsingHSV(FLinearColor(FreeColor), FLinearColor(FixedColor), Ratio);

                DrawDebugLine(GetWorld(), Node.Position, Neighbor.Position, LineColor.ToFColor(true), false, -1.f, 0, DebugLineThickness);
            }
        }
    }
}