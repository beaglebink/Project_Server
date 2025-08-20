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

bool UTeleportingComponent::IsActorFree() const
{
	return ActorIsFree;
}

void UTeleportingComponent::SetActorFree(bool bIsFree)
{
	ActorIsFree = bIsFree;
	if (bIsFree)
	{
		UE_LOG(LogTemp, Log, TEXT("Actor %s is now free for teleportation."), *GetOwner()->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Actor %s is now busy and cannot be teleported."), *GetOwner()->GetName());
	}
}
