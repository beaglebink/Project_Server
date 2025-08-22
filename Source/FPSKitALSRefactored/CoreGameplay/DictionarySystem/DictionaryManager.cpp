#include "DictionaryManager.h"
#include <FPSKitALSRefactored/CoreGameplay/TeleportationSystem/SceneDataProvider.h>



void ADictionaryManager::BeginPlay()
{
	Super::BeginPlay();

	//GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &ADictionaryManager::Initialize, 0.5f, true);
	Initialize();
}

void ADictionaryManager::Initialize()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogTemp, Warning, TEXT("TeleportingSubsystem: GameInstance not found"));
		return;
	}

	if (!GI->Implements<USceneDataProvider>())
	{
		UE_LOG(LogTemp, Warning, TEXT("GameInstance does not implement SceneDataProvider"));
		return;
	}

	DictionaryActorsTable = ISceneDataProvider::Execute_DictionaryDataTable(GI);

	if (!DictionaryActorsTable || !DictionaryActorsTable->RowStruct)
	{
		UE_LOG(LogTemp, Error, TEXT("DictionaryActorsTable is invalid or missing RowStruct"));
		return;
	}

	if (DictionaryActorsTable)
	{
		UE_LOG(LogTemp, Log, TEXT("DictionaryActorsTable loaded: %s"), *DictionaryActorsTable->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load DictionaryActorsTable"));
	}
/*
	TArray<FName> RowNames = DictionaryActorsTable->GetRowNames();

	if (RowNames.Num() != RegisteredKeysActors.Num())
		return;

	//TimerHandle.Invalidate();

	for (const FName& RowName : RowNames)
	{
		const FDictionaryActorStruct* Row = DictionaryActorsTable->FindRow<FDictionaryActorStruct>(RowName, TEXT("Dictionary system Row Lookup"));

		if (!Row)
		{
			UE_LOG(LogTemp, Warning, TEXT("Row %s not found in DictionaryActorsTable"), *RowName.ToString());
			continue;
		}

		FName ActorId = Row->ActorID;
		FName PropertyName = Row->PropertyName;
		FVariantProperty PropertyValue = Row->PropertyValue;

		for (ADictionaryObjectBase* KeysActor : RegisteredKeysActors)
		{
			if (KeysActor)
			{
				AKeysActor* Keys = Cast<AKeysActor>(KeysActor);

				if (!Keys)
				{
					UE_LOG(LogTemp, Warning, TEXT("Actor %s is not a valid KeysActor"), *KeysActor->GetName());
					continue;
				}

				if (Keys->KeyActorName != ActorId)
				{
					UE_LOG(LogTemp, Warning, TEXT("Keys Actor %s does not match ID: %s"), *KeysActor->GetName(), *ActorId.ToString());
					continue;
				}
				Keys->ApplyProperty(PropertyName, PropertyValue);
				UE_LOG(LogTemp, Log, TEXT("Applied property %s to Keys Actor: %s"), *PropertyName.ToString(), *KeysActor->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("No matching Keys Actor found for ID: %s"), *ActorId.ToString());
			}
		}
	}
*/
}

void ADictionaryManager::RegisterKeysActor(ADictionaryObjectBase* KeysActor)
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

void ADictionaryManager::UnregisterKeysActor(ADictionaryObjectBase* KeysActor)
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

void ADictionaryManager::RegisterPropertyActor(ADictionaryObjectBase* PropertyActor)
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

void ADictionaryManager::UnregisterPropertyActor(ADictionaryObjectBase* PropertyActor)
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

void ADictionaryManager::InitializeKeyActor(AKeysActor* KeyActor)
{
	if (!KeyActor)
	{
		return;
	}

	TArray<FName> RowNames = DictionaryActorsTable->GetRowNames();

	for (const FName& RowName : RowNames)
	{
		const FDictionaryActorStruct* Row = DictionaryActorsTable->FindRow<FDictionaryActorStruct>(RowName, TEXT("Dictionary system Row Lookup"));

		if (!Row)
		{
			UE_LOG(LogTemp, Warning, TEXT("Row %s not found in DictionaryActorsTable"), *RowName.ToString());
			continue;
		}

		FName TableActorId = Row->ActorID;
		FName PropertyName = Row->PropertyName;
		FVariantProperty PropertyValue = Row->PropertyValue;

		if (KeyActor->KeyActorName != TableActorId)
		{
			UE_LOG(LogTemp, Warning, TEXT("Keys Actor %s does not match ID: %s"), *KeyActor->GetName(), *TableActorId.ToString());
			continue;
		}
		KeyActor->ApplyProperty(PropertyName, PropertyValue);
		UE_LOG(LogTemp, Log, TEXT("Applied property %s to Keys Actor: %s"), *PropertyName.ToString(), *KeyActor->GetName());
	}
}
