#include "PythonContainers/A_InteractableActor.h"
#include "PythonContainers/A_ArrayEffect.h"
#include "Kismet/KismetMathLibrary.h"

AA_InteractableActor::AA_InteractableActor()
{
	PrimaryActorTick.bCanEverTick = true;
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	RootComponent = StaticMesh;

	StaticMesh->SetCollisionProfileName(TEXT("HighlyReactiveObject"));
	StaticMesh->SetSimulatePhysics(true);
	StaticMesh->SetNotifyRigidBodyCollision(true);
	StaticMesh->BodyInstance.SetMassOverride(7.0f, true);
	StaticMesh->SetLinearDamping(0.5f);
	StaticMesh->SetAngularDamping(0.1f);
}

void AA_InteractableActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AA_InteractableActor::BeginPlay()
{
	Super::BeginPlay();
}

void AA_InteractableActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool AA_InteractableActor::ParseAssignCommand(FText Command, FName& OutVarName, FName& OutActorName)
{
	FString Input = Command.ToString();

	FString Left, Right;

	if (!Input.Split(TEXT("="), &Left, &Right))
	{
		return false;
	}
	Left.RemoveSpacesInline();
	Right = Right.TrimStartAndEnd();

	if (!AA_PythonContainer::IsValidPythonIdentifier(Left))
	{
		return false;
	}

	OutVarName = FName(Left);
	OutActorName = FName(Right);

	return true;
}

void AA_InteractableActor::PortalInteract_Implementation(const FHitResult& Hit, const FTransform& EnterTransform, const FTransform& ExitTransform)
{
	FVector DeltaLocationExitToEnter = ExitTransform.GetLocation() - EnterTransform.GetLocation();
	FRotator DeltaRotationExitToEnter = UKismetMathLibrary::NormalizedDeltaRotator(ExitTransform.GetRotation().Rotator(), EnterTransform.GetRotation().Rotator());
	DeltaRotationExitToEnter.Yaw += 180.0f;
	SetActorLocation(GetActorLocation() + DeltaLocationExitToEnter, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(GetActorRotation() + DeltaRotationExitToEnter);

	if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(GetRootComponent()))
	{
		if (Prim->IsSimulatingPhysics())
		{
			FVector Velocity = Prim->GetPhysicsLinearVelocity();
			Prim->SetPhysicsLinearVelocity(DeltaRotationExitToEnter.RotateVector(Velocity));
		}
	}
}

void AA_InteractableActor::OnPowerConnected_Implementation(bool IsConnected)
{
}

FScannableActorData AA_InteractableActor::GetScannableObjectInfo_Implementation()
{
	return ScannableData;
}

void AA_InteractableActor::SetScannableObjectInfo_Implementation(const FScannableActorData& NewData)
{
	ScannableData = NewData;
}

UPrimitiveComponent* AA_InteractableActor::GetMeshComponent_Implementation()
{
	return StaticMesh;
}

void AA_InteractableActor::HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit)
{
	UE_LOG(LogTemp, Log, TEXT("AA_InteractableActor::HandleWeaponShot: Hit Actor: %s"), *GetNameSafe(Hit.GetActor()));
}

void AA_InteractableActor::HandleTextFromWeapon_Implementation(const FText& TextCommand)
{
	UE_LOG(LogTemp, Log, TEXT("AA_InteractableActor::HandleTextFromWeapon: %s"), *TextCommand.ToString());
}
