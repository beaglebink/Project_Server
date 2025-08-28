#include "NodeGridActor.h"
#include "DrawDebugHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include <Kismet/KismetMathLibrary.h>
#include <NiagaraDataInterfaceArrayFunctionLibrary.h>
#include "AlsCharacterExample_I.h"
//#include "NiagaraComponent.h"


ANodeGridActor::ANodeGridActor()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ANodeGridActor::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    //NiagaraComp = NewObject<UNiagaraComponent>(this, TEXT("NodeNiagara"));
    //NiagaraComp->SetupAttachment(RootComponent);
    //NiagaraComp->RegisterComponent();
    //NiagaraComp->SetAsset(NiagaraSystemAsset);

        //UNiagaraComponent* Niagara = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraSystemAsset, RootComponent, FName(TEXT("")), FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false, true);


}

void ANodeGridActor::BeginPlay()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ANodeGridActor::BeginPlay);

    Super::BeginPlay();

    GetWorldTimerManager().SetTimer(StunTimerHandle, this, &ANodeGridActor::DestroyThis, DesrtoyTime, false);

    Niagara = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraSystemAsset, RootComponent, FName(TEXT("")), FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false, true);


    InitializeGrid();


    /*
    for (const TPair<int32, int32>& Link : UniqueLinks)
    {
        Niagara = NewObject<UNiagaraComponent>(this, TEXT("NodeNiagara"));
        if (Niagara)
        {
            if (NiagaraSystemAsset)
            {
				Niagara->SetAsset(NiagaraSystemAsset);
				Niagara->SetupAttachment(RootComponent);
				Niagara->RegisterComponent();
				NiagaraComponents.Add(Niagara);
			}
        }
    }
    */
}

void ANodeGridActor::DestroyThis()
{
	DestroyNet(this);
}

void ANodeGridActor::DestroyNet(AActor* Reason)
{
    if (Reason != this) return;
	this->Destroy();
}

void ANodeGridActor::InitializeGrid()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ANodeGridActor::InitializeGrid);
    Nodes.Empty();
    NodePositions.Empty();
    RibbonStartIndices.Reset();
    RibbonEndIndices.Reset();

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
            FVector W_Velocity = UKismetMathLibrary::TransformDirection(GetActorTransform(), FVector(0.f, 0.f, Vel));

            int32 Index = NodePositions.Num();
            NodePositions.Add(Pos);

            Nodes.Add(FNode{ Index, Pos, W_Velocity, FVector::ZeroVector, false });
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
                float Rest = (NodePositions[Right] - NodePositions[Index]).Size();

                Nodes[Index].Links.Add(FNodeLink{ Right, Rest, CritLen, Stiffness, InfluenceAttenuation });
                Nodes[Right].Links.Add(FNodeLink{ Index, Rest, CritLen, Stiffness, InfluenceAttenuation });
            }

            if (y < TotalRows - 1)
            {
                int32 Down = Index + TotalCols;
                float Rest = (NodePositions[Down] - NodePositions[Index]).Size();

                Nodes[Index].Links.Add(FNodeLink{ Down, Rest, CritLen, Stiffness, InfluenceAttenuation });
                Nodes[Down].Links.Add(FNodeLink{ Index, Rest, CritLen, Stiffness, InfluenceAttenuation });
            }
        }
    }

    // 🧩 Сохранение уникальных связей для риббонов
    

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        const FNode& Node = Nodes[i];
        for (const FNodeLink& Link : Node.Links)
        {
            int32 j = Link.NeighborIndex;
            if (!Nodes.IsValidIndex(j)) continue;

            int32 A = FMath::Min(i, j);
            int32 B = FMath::Max(i, j);
            TPair<int32, int32> Key(A, B);

            if (!UniqueLinks.Contains(Key))
            {
                UniqueLinks.Add(Key);
                RibbonStartIndices.Add(A);
                RibbonEndIndices.Add(B);

                if (!bEnableDebugDraw)
                {
				    StartPositions.Add(NodePositions[A]);
                    FinishPositions.Add(NodePositions[B]);
                }
            }
        }
    }

    if (!bEnableDebugDraw && Niagara)
    {
        UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(Niagara, FName("StartPositions"), StartPositions);
        UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(Niagara, FName("FinishPositions"), FinishPositions);
    }

    /*
    if (NiagaraComp)
    {
        UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayInt32(
            NiagaraComp, FName("RibbonStart"), RibbonStartIndices);

        UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayInt32(
            NiagaraComp, FName("RibbonEnd"), RibbonEndIndices);
    }
    */
}

void ANodeGridActor::ApplyForcesParallel()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ANodeGridActor::ApplyForcesParallel);

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

        //if (Node.bFixed || Node.Velocity.Length() <= StopVelocity)
        //    StopCount++;

        if (Node.bFixed) continue;
        Node.AccumulatedForce += LocalForces[i];
    }
}

void ANodeGridActor::ProcessInfluenceCascade()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ANodeGridActor::ProcessInfluenceCascade);

    TQueue<FInfluenceEntry> InfluenceQueue;

    for (const auto& Entry : PendingInfluences)
    {
        int32 Index;
        FVector Velocity;
        float Factor;
        Tie(Index, Velocity, Factor) = Entry;

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

                TPair<int32, int32> LinkKey = (Current.Index < NeighborIndex)
                    ? TPair<int32, int32>(Current.Index, NeighborIndex)
                    : TPair<int32, int32>(NeighborIndex, Current.Index);

                if (!VisitedLinks.Contains(LinkKey))
                {
                    VisitedLinks.Add(LinkKey);
                    InfluenceQueue.Enqueue({ NeighborIndex, Current.Velocity, Current.Factor * InfluenceScale * InfluenceAttenuation });
                }
            }
        }
    }
}

void ANodeGridActor::ApplyMotionAndFixation(float DeltaTime)  
{  
    TRACE_CPUPROFILER_EVENT_SCOPE(ANodeGridActor::ApplyMotionAndFixation);

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
           Node.AttachedActor = Overlap.GetActor();
           UPrimitiveComponent* HitComp = Overlap.Component.Get();  
           if (!HitComp) continue;  

           if (HitComp->IsA<UStaticMeshComponent>())  
           {  
               FixationFlags[i] = true; 
			   Node.AttachedComponent = HitComp;
               break;  
           }  

           ACharacter* Char = Cast<ACharacter>(HitComp->GetOwner());  
           if (!Char) continue;  

		   ParalyzeCharacter(Char);

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
               Node.AttachedActor = MeshOverlap.GetActor();
               if (MeshOverlap.Component.Get() == FixationMesh)  
               {  
                   FTransform NodeTransform = FTransform(FRotator::ZeroRotator, NodePositions[i]/*TestPosition*/);

				   Node.AttachedMesh = FixationMesh;
				   FName ClosestBone = FindClosestBoneToPoint(FixationMesh, NodePositions[i]);
				   Node.AttachedBoneName = ClosestBone;
				   FTransform BoneTransform = FixationMesh->GetSocketTransform(ClosestBone, ERelativeTransformSpace::RTS_World);
				   Node.NodeLocalTransform = NodeTransform.GetRelativeTransform(BoneTransform);

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

FName ANodeGridActor::FindClosestBoneToPoint(USkeletalMeshComponent* SkeletalMesh, const FVector& Point) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ANodeGridActor::FindClosestBoneToPoint);

    if (!SkeletalMesh || !SkeletalMesh->GetSkeletalMeshAsset())
    {
        return NAME_None;
    }

    int32 NumBones = SkeletalMesh->GetNumBones();
    float MinDistSqr = FLT_MAX;
    FName ClosestBone = NAME_None;

    for (int32 i = 0; i < NumBones; ++i)
    {
        FName BoneName = SkeletalMesh->GetBoneName(i);
        FVector BoneWorldPos = SkeletalMesh->GetBoneLocation(BoneName);

        float DistSqr = FVector::DistSquared(BoneWorldPos, Point);
        if (DistSqr < MinDistSqr)
        {
            MinDistSqr = DistSqr;
            ClosestBone = BoneName;
        }
    }

    return ClosestBone;
}

void ANodeGridActor::ParalyzeCharacter(ACharacter* Char)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ANodeGridActor::ParalyzeCharacter);

	if (!Char) return;
	if (ParalysedCharacters.Contains(Cast<AAlsCharacterExample>(Char))) return;
    
    AAlsCharacterExample* AlsCharacter = Cast<AAlsCharacterExample>(Char);
	if (AlsCharacter)
	{
		float ParalyseTime = AlsCharacter->GetNetGrenadeParalyseTime();
		AAlsCharacterExample* ALSChar = Cast<AAlsCharacterExample>(Char);
		if (ALSChar)
		{
			ALSChar->ParalyzeNPC(this, ParalyseTime);
			ALSChar->OnNetParalyse.AddDynamic(this, &ANodeGridActor::DestroyNet);
		}

		ParalysedCharacters.Add(AlsCharacter);
	}
}

void ANodeGridActor::ApplyRigidConstraints(float DeltaTime)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ANodeGridActor::ApplyRigidConstraints);

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

void ANodeGridActor::DrawState()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ANodeGridActor::DrawState);

    //const float SPart = float(StopCount) / float(Nodes.Num());
    const float DrawLifeTime = -1;//SPart >= StopTresholdPart ? 60.f : -1.f;

    //if (!bEnableDebugDraw) return;

    const FColor FreeColor = FColor::Green;
    const FColor FixedColor = FColor::Red;

    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        const FNode& Node = Nodes[i];
        const FColor NodeColor = Node.bFixed ? FixedColor : FreeColor;
        const FVector& Pos = NodePositions[Node.PositionIndex];

        if (bEnableDebugDraw)
        {
            DrawDebugSphere(GetWorld(), Pos, DebugSphereRadius, 12, NodeColor, false, DrawLifeTime, 0, 0.5f);
        }
      

        for (const FNodeLink& Link : Node.Links)
        {
            if (!Nodes.IsValidIndex(Link.NeighborIndex)) continue;
            const FNode& Neighbor = Nodes[Link.NeighborIndex];

            float CurrentLength = (NodePositions[Neighbor.PositionIndex] - Pos).Size();
            float Ratio = FMath::Clamp(CurrentLength / Link.CriticalLength, 0.f, RCorrect);
            FLinearColor LineColor = FLinearColor::LerpUsingHSV(FLinearColor(FreeColor), FLinearColor(FixedColor), Ratio);

            if (bEnableDebugDraw)
            {
                DrawDebugLine(GetWorld(), Pos, NodePositions[Neighbor.PositionIndex], LineColor.ToFColor(true), false, DrawLifeTime, 0, DebugLineThickness);
            }
            else
            {
				StartPositions[Node.PositionIndex] = Pos;
				FinishPositions[Neighbor.PositionIndex] = NodePositions[Neighbor.PositionIndex];
            }
        }

        if (!bEnableDebugDraw && Niagara)
        {
            UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(Niagara, FName("StartPositions"), StartPositions);
            UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(Niagara, FName("FinishPositions"), FinishPositions);
        }

    }
}



void ANodeGridActor::IterateUniqueLinks()
{
    /*
    int32 ID = 0;

    //UNiagaraComponent* Niagara = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraSystemAsset, RootComponent, FName(TEXT("")), FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false, true);

    for (const TPair<int32, int32>& Link : UniqueLinks)
    {
        int32 StartIndex = Link.Key;
        int32 EndIndex = Link.Value;

		UNiagaraComponent* Niagara = NiagaraComponents.IsValidIndex(ID) ? NiagaraComponents[ID] : nullptr;

        if (Niagara)
        {
            FVector StartPos = NodePositions[StartIndex];
            FVector EndPos = NodePositions[EndIndex];
            TArray<FVector> Positions;
            Positions.Add(StartPos);
            Positions.Add(EndPos);

            UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(Niagara, FName("Positions"), Positions);
            Niagara->SetVariableInt(TEXT("RibID"), ID);

            ID++;
        }
    }
    */
}

void ANodeGridActor::Tick(float DeltaTime)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(ANodeGridActor::Tick);

    Super::Tick(DeltaTime);

    StopCount = 0;
    PendingInfluences.Reset();

    ApplyForcesParallel();
    ProcessInfluenceCascade();
    ApplyMotionAndFixation(DeltaTime);
    ApplyRigidConstraints(DeltaTime);
    DrawState();

    // Подготовка временных массивов для параллельной обработки
    TArray<FVector> UpdatedPositions;
    TArray<bool> Updated;
    TArray<bool> SetFixation;
    UpdatedPositions.SetNumUninitialized(Nodes.Num());
    Updated.SetNumZeroed(Nodes.Num());
	SetFixation.SetNumZeroed(Nodes.Num());

    // Параллельная обработка фиксированных узлов
    ParallelFor(Nodes.Num(), [&](int32 i)
    {
        FNode& Node = Nodes[i];

        if (Node.bFixed)
        {

            if (!IsValid(Node.AttachedActor))
            {
				Updated[i] = true;
				SetFixation[i] = false;
                return;
            }

            if (!Node.AttachedComponent && !Node.AttachedMesh)
            {
                Updated[i] = true;
                SetFixation[i] = false;
                return;
            }

            AAlsCharacter* Char = Cast<AAlsCharacter>(Node.AttachedActor);
            
            if (!Char)
            {
                Updated[i] = false;
                return;
            }

            float Health = Char->GetHealth();
            if (Health <= 0.f || Char->IsRagdollingAllowedToStop())
            {
                Updated[i] = true;
                SetFixation[i] = false;
                return;
            }

            FTransform BoneTransform = Node.AttachedMesh->GetSocketTransform(Node.AttachedBoneName, RTS_World);
            FTransform WorldTransform = Node.NodeLocalTransform * BoneTransform;

            UpdatedPositions[i] = WorldTransform.GetLocation();
            Updated[i] = true;
            SetFixation[i] = true;

        }
    });

    // Применение результатов в основном потоке
    for (int32 i = 0; i < Nodes.Num(); ++i)
    {
        FNode& Node = Nodes[i];

        if (Updated[i])
        {
            if (SetFixation[i])
            {
                NodePositions[Node.PositionIndex] = UpdatedPositions[i];
                Node.Velocity = FVector::ZeroVector;
                Node.bFixed = true;
            }
            else
            {
                Node.Velocity = FVector::ZeroVector;
                Node.bFixed = false;
            }
        }
        /*
        else if (Node.bFixed)
        {
            Node.Velocity = FVector::ZeroVector;
            Node.bFixed = false;
        }
        */
    }
}