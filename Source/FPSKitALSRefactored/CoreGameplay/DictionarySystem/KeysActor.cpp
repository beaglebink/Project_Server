#include "KeysActor.h"
#include "DictionaryManager.h"

void AKeysActor::BeginPlay()
{
	Super::BeginPlay();
	if (ManagerInstance)
	{
		// Register this actor with the dictionary manager
		KeyValues.Empty();
		ManagerInstance->RegisterKeysActor(this);
		ManagerInstance->InitializeKeyActor(this);
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
void AKeysActor::AddPropertyDescription(const FName PropertyName, const FName ValueName)
{
	
}

void AKeysActor::ApplyProperty_Implementation(const FName PropertyName, const FVariantProperty Value)
{
}

