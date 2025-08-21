#include "DictionaryManager.h"

void ADictionaryManager::RegisterKeysActor(ADictionaryOnjectBase* KeysActor)
{
	if (KeysActor && !RegisteredKeysActors.Contains(KeysActor))
	{
		RegisteredKeysActors.Add(KeysActor);
		UE_LOG(LogTemp, Log, TEXT("Registered Keys Actor: %s"), *KeysActor->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to register Keys Actor: %s"), *KeysActor->GetName());
	}
}

void ADictionaryManager::UnregisterKeysActor(ADictionaryOnjectBase* KeysActor)
{
	if (KeysActor && RegisteredKeysActors.Contains(KeysActor))
	{
		RegisteredKeysActors.Remove(KeysActor);
		UE_LOG(LogTemp, Log, TEXT("Unregistered Keys Actor: %s"), *KeysActor->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to unregister Keys Actor: %s"), *KeysActor->GetName());
	}
}

void ADictionaryManager::RegisterPropertyActor(ADictionaryOnjectBase* PropertyActor)
{
	if (PropertyActor && !RegisteredPropertyActors.Contains(PropertyActor))
	{
		RegisteredPropertyActors.Add(PropertyActor);
		UE_LOG(LogTemp, Log, TEXT("Registered Property Actor: %s"), *PropertyActor->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to register Property Actor: %s"), *PropertyActor->GetName());
	}
}

void ADictionaryManager::UnregisterPropertyActor(ADictionaryOnjectBase* PropertyActor)
{
	if (PropertyActor && RegisteredPropertyActors.Contains(PropertyActor))
	{
		RegisteredPropertyActors.Remove(PropertyActor);
		UE_LOG(LogTemp, Log, TEXT("Unregistered Property Actor: %s"), *PropertyActor->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to unregister Property Actor: %s"), *PropertyActor->GetName());
	}
}
