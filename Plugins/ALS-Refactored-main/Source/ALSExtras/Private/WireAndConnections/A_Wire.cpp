#include "WireAndConnections/A_Wire.h"
#include "CableComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "WireAndConnections/A_WireConnector.h"
#include "ItemPlacement/A_DropZone.h"

AA_Wire::AA_Wire()
{
	PrimaryActorTick.bCanEverTick = true;

	CableComponent = CreateDefaultSubobject<UCableComponent>(TEXT("CableComponent"));
	Constraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("Constraint"));

	CableComponent->AttachToComponent(StaticMesh, FAttachmentTransformRules::KeepRelativeTransform, "CableStart");
	Constraint->SetupAttachment(StaticMesh);

	CableComponent->EndLocation = FVector(0.0f, 0.0f, 0.0f);
	CableComponent->CableWidth = 2.0f;
	CableComponent->NumSegments = 20;
	CableComponent->bEnableStiffness = true;
	CableComponent->bSkipCableUpdateWhenNotVisible = true;
	CableComponent->bEnableCollision = true;

	Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 90.0f);
	Constraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 90.0f);
	Constraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 5.0f);
}

void AA_Wire::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITOR
	if (!IsValid(OppositeConnector))
	{
		if (OppositeConnectorClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("Spawning OppositeConnector"));
			OppositeConnector = GetWorld()->SpawnActor<AA_WireConnector>(OppositeConnectorClass, GetActorLocation() + GetActorForwardVector() * 250.0f, GetActorRotation());
		}
	}

	if (!IsValid(OppositeConnection))
	{
		if (OppositeConnectionClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("Spawning OppositeConnection"));
			OppositeConnection = GetWorld()->SpawnActor<AA_DropZone>(OppositeConnectionClass, OppositeConnector->GetActorTransform());
		}
	}
#endif

	Name = "Wire male";
	if (IsValid(OppositeConnector))
	{
		OppositeConnector->OppositeConnector = this;
		OppositeConnector->Name = "Wire female";
		if (OppositeConnectorMesh)
		{
			OppositeConnector->StaticMesh->SetStaticMesh(OppositeConnectorMesh);
		}
		CableComponent->SetAttachEndToComponent(OppositeConnector->StaticMesh, "CableSocket");
		CableComponent->CableLength = FVector::Distance(GetActorLocation(), OppositeConnector->GetActorLocation()) + 50.0f;

		Constraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Limited, CableComponent->CableLength);
		Constraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Limited, CableComponent->CableLength);
		Constraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Limited, CableComponent->CableLength);
	}

	if (IsValid(OppositeConnection))
	{
		OppositeConnector->OppositeConnection = OppositeConnection;
		OppositeConnection->AttachToActor(OppositeConnector, FAttachmentTransformRules::SnapToTargetIncludingScale, "ConnectorSocket");
	}
}

void AA_Wire::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_Wire::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(OppositeConnector))
	{
		Constraint->SetWorldLocation((GetActorLocation() + OppositeConnector->GetActorLocation()) / 2.0f);
		Constraint->SetConstrainedComponents(StaticMesh, NAME_None, OppositeConnector->StaticMesh, NAME_None);
		CableComponent->SetAttachEndToComponent(OppositeConnector->StaticMesh, "CableSocket");
	}
}