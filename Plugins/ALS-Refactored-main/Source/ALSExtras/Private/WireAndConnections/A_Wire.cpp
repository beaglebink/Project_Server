#include "WireAndConnections/A_Wire.h"
#include "CableComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"


AA_Wire::AA_Wire()
{
	PrimaryActorTick.bCanEverTick = true;

	FemaleMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FemaleMeshComponent"));
	CableComponent = CreateDefaultSubobject<UCableComponent>(TEXT("CableComponent"));
	ConstraintStartMiddle = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("ConstraintStartMiddle"));
	ConstraintMiddleEnd = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("ConstraintMiddleEnd"));
	MiddlePoint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MiddlePoint"));

	FemaleMeshComponent->SetupAttachment(StaticMesh);
	MiddlePoint->SetupAttachment(StaticMesh);
	CableComponent->AttachToComponent(StaticMesh, FAttachmentTransformRules::KeepRelativeTransform, "CableStart");
	ConstraintStartMiddle->SetupAttachment(StaticMesh);
	ConstraintMiddleEnd->SetupAttachment(MiddlePoint);

	FemaleMeshComponent->SetCollisionProfileName(TEXT("HighlyReactiveObject"));
	FemaleMeshComponent->SetSimulatePhysics(true);
	FemaleMeshComponent->SetNotifyRigidBodyCollision(true);
	FemaleMeshComponent->BodyInstance.SetMassOverride(7.0f, true);
	FemaleMeshComponent->SetLinearDamping(0.5f);
	FemaleMeshComponent->SetAngularDamping(0.1f);

	MiddlePoint->bHiddenInGame = true;
	MiddlePoint->SetCollisionProfileName(TEXT("HighlyReactiveObject"));
	MiddlePoint->SetSimulatePhysics(true);
	MiddlePoint->SetNotifyRigidBodyCollision(true);
	MiddlePoint->BodyInstance.SetMassOverride(7.0f, true);
	MiddlePoint->SetLinearDamping(0.5f);
	MiddlePoint->SetAngularDamping(0.1f);

	CableComponent->EndLocation = FVector(0.0f, 0.0f, 0.0f);
	CableComponent->CableWidth = 2.0f;

	ConstraintStartMiddle->SetConstrainedComponents(StaticMesh, NAME_None, MiddlePoint, NAME_None);
	ConstraintStartMiddle->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 90.0f);
	ConstraintStartMiddle->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 90.0f);
	ConstraintStartMiddle->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 5.0f);

	ConstraintMiddleEnd->SetConstrainedComponents(MiddlePoint, NAME_None, FemaleMeshComponent, NAME_None);
	ConstraintMiddleEnd->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Free, 90.0f);
	ConstraintMiddleEnd->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Free, 90.0f);
	ConstraintMiddleEnd->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Free, 5.0f);
}

void AA_Wire::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	MiddlePoint->SetRelativeLocation(FemaleMeshComponent->GetRelativeLocation() / 2.0f);

	CableComponent->SetAttachEndToComponent(FemaleMeshComponent, "CableEnd");
	CableComponent->CableLength = FemaleMeshComponent->GetRelativeLocation().Length() + 50.0f;

	ConstraintStartMiddle->SetLinearXLimit(ELinearConstraintMotion::LCM_Limited, CableComponent->CableLength / 2.0f);
	ConstraintStartMiddle->SetLinearYLimit(ELinearConstraintMotion::LCM_Free, CableComponent->CableLength / 2.0f);
	ConstraintStartMiddle->SetLinearZLimit(ELinearConstraintMotion::LCM_Free, CableComponent->CableLength / 2.0f);

	ConstraintMiddleEnd->SetLinearXLimit(ELinearConstraintMotion::LCM_Limited, CableComponent->CableLength / 2.0f);
	ConstraintMiddleEnd->SetLinearYLimit(ELinearConstraintMotion::LCM_Free, CableComponent->CableLength / 2.0f);
	ConstraintMiddleEnd->SetLinearZLimit(ELinearConstraintMotion::LCM_Free, CableComponent->CableLength / 2.0f);
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