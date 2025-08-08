#include "TeleportDestination.h"
#include "TeleportingSubsystem.h"

UTeleportingSubsystem* GetTeleportingSubsystem(UObject* Context)
{
	if (!Context) return nullptr;

	UWorld* World = Context->GetWorld();
	if (!World) return nullptr;

	UGameInstance* GI = World->GetGameInstance();
	if (!GI) return nullptr;

	return GI->GetSubsystem<UTeleportingSubsystem>();
}

void ATeleportDestination::BeginPlay()
{
	Super::BeginPlay();

	UTeleportingSubsystem* TeleportSubsystem = GetTeleportingSubsystem(this);

	// Register this teleport destination with the teleporting subsystem

	if (TeleportSubsystem)
	{
		TeleportSubsystem->RegistrationTeleportingDestination(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TeleportingSubsystem not found!"));
	}

}

void ATeleportDestination::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	// Unregister this teleport destination from the teleporting subsystem

	UTeleportingSubsystem* TeleportSubsystem = GetTeleportingSubsystem(this);

	if (TeleportSubsystem)
	{
		TeleportSubsystem->UnregistrationTeleportingDestination(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("TeleportingSubsystem not found during EndPlay!"));
	}

}


