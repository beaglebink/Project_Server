#include "DebrisSystem/AC_DebrisSpawnComponent.h"
#include "Kismet/KismetMathLibrary.h"

UAC_DebrisSpawnComponent::UAC_DebrisSpawnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

#if WITH_EDITOR
void UAC_DebrisSpawnComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FlushPersistentDebugLines(GetWorld());

	if (!GetOwner()) return;

	const FVector WorldLocation = GetOwner()->GetActorLocation();
	const FRotator WorldRotation = GetOwner()->GetActorRotation();

	switch (SpawnShape)
	{
	case EDebrisSpawnShape::Sphere:
	{
		DrawDebugSphere(GetWorld(), WorldLocation, SphereRadius, 16, FColor::Green, true, -1.0f, 0, 2.0f);
		break;
	}
	case EDebrisSpawnShape::Capsule:
	{
		DrawDebugCapsule(GetWorld(), WorldLocation, CapsuleHalfHeight, SphereRadius, WorldRotation.Quaternion(), FColor::Green, true, -1.0f, 0, 2.0f);
		break;
	}
	case EDebrisSpawnShape::Disc:
	{
		DrawDebugCircle(GetWorld(), WorldLocation, SphereRadius, 32, FColor::Green, true, -1.0f, 0, 2.0f, WorldRotation.Vector(), FVector::RightVector, false);
		break;
	}
	case EDebrisSpawnShape::Plane:
	{
		const FVector Right = WorldRotation.RotateVector(FVector::RightVector) * PlaneSize.Y;
		const FVector Forward = WorldRotation.RotateVector(FVector::ForwardVector) * PlaneSize.X;

		DrawDebugLine(GetWorld(), WorldLocation - Right - Forward, WorldLocation + Right - Forward, FColor::Green, true, -1.0f, 0, 2.0f);
		DrawDebugLine(GetWorld(), WorldLocation + Right - Forward, WorldLocation + Right + Forward, FColor::Green, true, -1.0f, 0, 2.0f);
		DrawDebugLine(GetWorld(), WorldLocation + Right + Forward, WorldLocation - Right + Forward, FColor::Green, true, -1.0f, 0, 2.0f);
		DrawDebugLine(GetWorld(), WorldLocation - Right + Forward, WorldLocation - Right - Forward, FColor::Green, true, -1.0f, 0, 2.0f);
		break;
	}
	case EDebrisSpawnShape::Box:
	{
		DrawDebugBox(GetWorld(), WorldLocation, BoxExtent, WorldRotation.Quaternion(), FColor::Green, true, -1.0f, 0, 2.0f);
		break;
	}
	default:
		break;
	}
}
#endif

void UAC_DebrisSpawnComponent::BeginPlay()
{
	Super::BeginPlay();

	SpawnDebris();
}

void UAC_DebrisSpawnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

FVector UAC_DebrisSpawnComponent::GetRandomPointInVolume() const
{
	if (!GetOwner()) return FVector::ZeroVector;

	FVector LocalPoint = FVector::ZeroVector;

	switch (SpawnShape)
	{
	case EDebrisSpawnShape::Sphere:
	{
		LocalPoint = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.0f, SphereRadius);
		break;
	}
	case EDebrisSpawnShape::Capsule:
	{
		float Z = FMath::FRandRange(-CapsuleHalfHeight, CapsuleHalfHeight);
		FVector2D Circle = FVector2D(UKismetMathLibrary::RandomUnitVector()) * FMath::FRandRange(0.0f, SphereRadius);
		LocalPoint = FVector(Circle.X, Circle.Y, Z);
		break;
	}
	case EDebrisSpawnShape::Box:
	{
		LocalPoint = FVector(FMath::FRandRange(-BoxExtent.X, BoxExtent.X), FMath::FRandRange(-BoxExtent.Y, BoxExtent.Y), FMath::FRandRange(-BoxExtent.Z, BoxExtent.Z));
		break;
	}
	case EDebrisSpawnShape::Plane:
	{
		LocalPoint = FVector(FMath::FRandRange(-PlaneSize.X * 0.5f, PlaneSize.X * 0.5f), FMath::FRandRange(-PlaneSize.Y * 0.5f, PlaneSize.Y * 0.5f), 0.0f);
		break;
	}
	case EDebrisSpawnShape::Disc:
	{
		FVector2D Circle = FVector2D(UKismetMathLibrary::RandomUnitVector()) * FMath::FRandRange(0.0f, SphereRadius);
		LocalPoint = FVector(Circle.X, Circle.Y, 0.0f);
		break;
	}
	default:
		break;
	}

	LocalPoint *= SpawnDensity;

	return GetOwner()->GetActorTransform().TransformPosition(LocalPoint);
}

void UAC_DebrisSpawnComponent::SpawnDebris()
{
	if (!GetOwner() || !DebrisClass || DebrisMeshes.Num() == 0) return;

	for (size_t i = 0; i < DebrisCount; ++i)
	{
		const FTransform SpawnTransform(UKismetMathLibrary::RandomRotator(true), GetRandomPointInVolume());
		AA_Debris* SpawnedDebris = GetWorld()->SpawnActorDeferred<AA_Debris>(DebrisClass, SpawnTransform, GetOwner(), GetOwner()->GetInstigator());
		if (SpawnedDebris)
		{
			const int32 RandomMeshIndex = FMath::RandRange(0, DebrisMeshes.Num() - 1);
			SpawnedDebris->FindComponentByClass<UStaticMeshComponent>()->SetStaticMesh(DebrisMeshes[RandomMeshIndex]);

			SpawnedDebris->bShouldMeshSimulatePhysics = bShouldMeshSimulatePhysics;
			SpawnedDebris->ReplenishTime = ReplenishTime;
			SpawnedDebris->SuctionDifficultyMultiplier = SuctionDifficultyMultiplier;
			SpawnedDebris->Slipperiness = Slipperiness;
			SpawnedDebris->SlipperinessImpulsePower = SlipperinessImpulsePower;
			SpawnedDebris->GarbagePercentage = GarbagePercentage;
			SpawnedDebris->DebrisBehavior = DebrisBehavior;
			SpawnedDebris->DebrisMovementSpeed = DebrisMovementSpeed;
			SpawnedDebris->PlayerReaction = PlayerReaction;
			SpawnedDebris->PlayerInfluenceStrength = PlayerInfluenceStrength;
			SpawnedDebris->PlayerInfluenceRadius = PlayerInfluenceRadius;
			SpawnedDebris->TurbulenceStrength = TurbulenceStrength;

			SpawnedDebris->FinishSpawning(SpawnTransform);
		}
	}
}

