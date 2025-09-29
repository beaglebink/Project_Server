#include "ArrayEffect/A_InteractableActor.h"
#include "ArrayEffect/A_ArrayEffect.h"
#include "Kismet/KismetMathLibrary.h"

AA_InteractableActor::AA_InteractableActor()
{
	PrimaryActorTick.bCanEverTick = true;
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

	if (!AA_ArrayEffect::IsValidPythonIdentifier(Left))
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
