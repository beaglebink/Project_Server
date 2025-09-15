#include "ArrayEffect/A_InteractableActor.h"
#include "ArrayEffect/A_ArrayEffect.h"

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