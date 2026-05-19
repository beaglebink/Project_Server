#include "WireAndConnections/A_Wire.h"
#include "CableComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"


AA_Wire::AA_Wire()
{
	PrimaryActorTick.bCanEverTick = true;

	FemaleMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FemaleMeshComponent"));
	CableComponent = CreateDefaultSubobject<UCableComponent>(TEXT("CableComponent"));
	Constraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("Constraint"));

	FemaleMeshComponent->SetupAttachment(StaticMesh);
	CableComponent->AttachToComponent(StaticMesh, FAttachmentTransformRules::KeepRelativeTransform, "CableStart");
	Constraint->SetupAttachment(StaticMesh);

	FemaleMeshComponent->SetCollisionProfileName(TEXT("HighlyReactiveObject"));
	FemaleMeshComponent->SetSimulatePhysics(true);
	FemaleMeshComponent->SetNotifyRigidBodyCollision(true);
	FemaleMeshComponent->BodyInstance.SetMassOverride(7.0f, true);
	FemaleMeshComponent->SetLinearDamping(0.5f);
	FemaleMeshComponent->SetAngularDamping(0.1f);

	CableComponent->EndLocation = FVector(0.0f, 0.0f, 0.0f);
	CableComponent->CableWidth = 2.0f;
	CableComponent->NumSegments = 20;
	CableComponent->bEnableStiffness = true;
	CableComponent->bSkipCableUpdateWhenNotVisible = true;
	CableComponent->bEnableCollision = true;


	Constraint->SetConstrainedComponents(StaticMesh, NAME_None, FemaleMeshComponent, NAME_None);

	Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 90.0f);
	Constraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 90.0f);
	Constraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 5.0f);
}

void AA_Wire::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	CableComponent->SetAttachEndToComponent(FemaleMeshComponent, "CableEnd");
	CableComponent->CableLength = FemaleMeshComponent->GetRelativeLocation().Length() + 50.0f;

	Constraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Limited, CableComponent->CableLength);
	Constraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Limited, CableComponent->CableLength);
	Constraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Limited, CableComponent->CableLength);
}

void AA_Wire::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_Wire::BeginPlay()
{
	Super::BeginPlay();

	CableComponent->SetAttachEndToComponent(FemaleMeshComponent, "CableEnd");
}