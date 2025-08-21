#include "PropertyActor.h"
#include "DictionaryManager.h"

void APropertyActor::BeginPlay()
{
	Super::BeginPlay();

	if (ManagerInstance)
	{
		// Register this actor with the dictionary manager
		ManagerInstance->RegisterPropertyActor(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PropertyActor: ManagerInstance is null!"));
	}
}

void APropertyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (ManagerInstance)
	{
		// Unregister this actor from the dictionary manager
		ManagerInstance->UnregisterPropertyActor(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PropertyActor: ManagerInstance is null during EndPlay!"));
	}
}
