#include "KeysActor.h"
#include "DictionaryManager.h"

void AKeysActor::BeginPlay()
{
	Super::BeginPlay();
	if (ManagerInstance)
	{
		// Register this actor with the dictionary manager
		ManagerInstance->RegisterKeysActor(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("KeysActor: ManagerInstance is null!"));
	}
}

void AKeysActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (ManagerInstance)
	{
		// Unregister this actor from the dictionary manager
		ManagerInstance->UnregisterKeysActor(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("KeysActor: ManagerInstance is null during EndPlay!"));
	}
}

void AKeysActor::ApplyProperty(const FName PropertyName, const FString& Value)
{
}
