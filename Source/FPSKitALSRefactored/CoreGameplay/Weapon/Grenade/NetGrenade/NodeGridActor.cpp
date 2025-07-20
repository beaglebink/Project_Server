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
            const int32 Index = y * TotalCols + x;

            FNode NewNode;
            FVector LocalOffset(x * CellSize - HalfX, y * CellSize - HalfY, 0);
            NewNode.Position = GetActorTransform().TransformPosition(LocalOffset);
            NewNode.Velocity = FVector::ZeroVector;
            NewNode.bFixed = false;

            if (x < GridCols)
            {
                FNodeLink RightLink;
                RightLink.NeighborIndex = Index + 1;
                RightLink.RestLength = CellSize;
                RightLink.CriticalLength = CritLen;
                RightLink.Stiffness = Stiffness;
                NewNode.Links.Add(RightLink);
            }

            if (y < GridRows)
            {
                FNodeLink DownLink;
                DownLink.NeighborIndex = Index + TotalCols;
                DownLink.RestLength = CellSize;
                DownLink.CriticalLength = CritLen;
                DownLink.Stiffness = Stiffness;
                NewNode.Links.Add(DownLink);
            }

            Nodes.Add(NewNode);
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

void ANodeGridActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    const FVector GravityForce = GravityDirection.GetSafeNormal() * GravityStrength;
    const FCollisionShape ProbeShape = FCollisionShape::MakeSphere(OverlapRadius);
    FCollisionObjectQueryParams ObjectParams(ECC_WorldStatic | ECC_PhysicsBody | ECC_WorldDynamic);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MeshFixationProbe), false, this);

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        FNode& Node = Nodes[i];
        if (Node.bFixed) continue;

        // Движение
        Node.AccumulatedForce += GravityForce;
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