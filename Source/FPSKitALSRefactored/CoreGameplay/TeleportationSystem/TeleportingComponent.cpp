#include "TeleportingComponent.h"
#include "TeleportingSubsystem.h"

UTeleportingComponent::UTeleportingComponent()
{
}

void UTeleportingComponent::BeginPlay()
{
	Super::BeginPlay();
	// Register this actor with the teleporting subsystem
	if (UTeleportingSubsystem* TeleportingSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UTeleportingSubsystem>())
	{
		TeleportingSubsystem->RegistrationTeleportingActor(GetOwner());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TeleportingSubsystem not found!"));
	}

}

void UTeleportingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	// Unregister this actor from the teleporting subsystem
	if (UTeleportingSubsystem* TeleportingSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UTeleportingSubsystem>())
	{
		TeleportingSubsystem->UnregistrationTeleportingActor(GetOwner());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TeleportingSubsystem not found during EndPlay!"));
	}
}
