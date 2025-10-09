#include "EnvironmentalHazard/A_BinaryCube.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractiveItemComponent.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractivePickerComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "AlsCameraComponent.h"

AA_BinaryCube::AA_BinaryCube()
{
	PrimaryActorTick.bCanEverTick = true;

	InteractiveComponent = CreateDefaultSubobject<UInteractiveItemComponent>(TEXT("InteractiveItemComponent"));
}

void AA_BinaryCube::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!MeshDynamicMaterial)
	{
		MeshDynamicMaterial = StaticMeshComp->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (MeshDynamicMaterial)
	{
		MeshDynamicMaterial->SetScalarParameterValue("Index", static_cast<int32>(bBinary));
	}

	InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to grab"));
}

void AA_BinaryCube::BeginPlay()
{
	Super::BeginPlay();

	InteractiveComponent->OnInteractionPressKeyEvent.AddDynamic(this, &AA_BinaryCube::GrabDropObject);
}

void AA_BinaryCube::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_BinaryCube::HandleWeaponShot_Implementation(FHitResult& Hit)
{
	if (bIsOnDeath)
	{
		return;
	}

	AudioComp->Play();

	bBinary = !bBinary;

	MeshDynamicMaterial->SetScalarParameterValue("Index", static_cast<int32>(bBinary));
}

void AA_BinaryCube::OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult)
{
	if (!bCanBeHit || bIsOnDeath || !OtherActor)
	{
		return;
	}

	Super::OnMeshHit(HitComp, OtherActor, OtherComp, NormalImpulse, HitResult);

	if (bBinary)
	{
		UGameplayStatics::ApplyDamage(OtherActor, DamageCaused, OtherActor->GetInstigatorController(), this, nullptr);
	}

	if (AA_BinaryCube* HitCube = Cast< AA_BinaryCube>(OtherActor))
	{
		if (bBinary == !HitCube->bBinary)
		{
			if (HitCube->StaticMeshComp->IsSimulatingPhysics())
			{
				HitCube->InteractiveComponent->OnInteractionPressKeyEvent.Broadcast(Grabber);
			}
			HitCube->OnDeath();
			if (StaticMeshComp->IsSimulatingPhysics())
			{
				InteractiveComponent->OnInteractionPressKeyEvent.Broadcast(Grabber);
			}
			OnDeath();
		}
	}
}

void AA_BinaryCube::GrabDropObject(UInteractivePickerComponent* Picker)
{
	Grabber = Picker;

	if (bGrabOrDrop)
	{
		InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to drop"));

		SetCubePhysic(true);
	}
	else
	{
		InteractiveComponent->InteractiveTooltipText = FText::FromString(TEXT("Press \"F\" to grab"));

		SetCubePhysic(false);
	}

	bGrabOrDrop = !bGrabOrDrop;
}

void AA_BinaryCube::SetCubePhysic(bool IsSet)
{
	if (IsSet)
	{
		StaticMeshComp->SetSimulatePhysics(true);
		StaticMeshComp->SetEnableGravity(false);
	}
	else
	{
		StaticMeshComp->SetSimulatePhysics(false);
		SetActorLocationAndRotation(StaticMeshComp->GetComponentLocation(), StaticMeshComp->GetComponentRotation());
		DefaultLocation = GetActorLocation();
		StaticMeshComp->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	}
}
