#include "DictionaryManager.h"
#include <FPSKitALSRefactored/CoreGameplay/TeleportationSystem/SceneDataProvider.h>
#include "PropertyActor.h"



void ADictionaryManager::BeginPlay()
{
	Super::BeginPlay();

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

		APropertyActor* PropertyActorCast = Cast<APropertyActor>(PropertyActor);

		
		FVariantProperty ActorPropertyValue = PropertyActorCast->Property;
		FName PropertyTypeName = ActorPropertyValue.VariableTypeName;
		FName PropertyValueName = ActorPropertyValue.ValueName;

		for (auto KeysActor : RegisteredKeysActors)
		{
			AKeysActor* KeysActorCast = Cast<AKeysActor>(KeysActor);

			if (!KeysActorCast)
			{
				continue;
			}

			TArray<FName> RowNames = DictionaryActorsTable->GetRowNames();

			for (const FName& RowName : RowNames)
			{
				const FDictionaryActorStruct* Row = DictionaryActorsTable->FindRow<FDictionaryActorStruct>(RowName, TEXT("Dictionary system Row Lookup"));

				if (!Row)
				{
					//UE_LOG(LogTemp, Warning, TEXT("Row %s not found in DictionaryActorsTable %s"), *RowName.ToString());
					continue;
				}

				FName TableActorId = Row->ActorID;
				FName TableTypeName = Row->PropertyValue.VariableTypeName;
				FName TablePropertyName = Row->PropertyName;
				FVariantProperty TablePropertyValue = Row->PropertyValue;
				FName TablePropertyValueName = TablePropertyValue.ValueName;

				if (KeysActorCast->KeyTypes.Contains(TablePropertyName))
				{
					if (KeysActorCast->KeyTypes.Find(TablePropertyName) && *KeysActorCast->KeyTypes.Find(TablePropertyName) == PropertyTypeName)
					{
						if (KeysActorCast->KeyValues.Contains(TablePropertyName))
						{
							if (KeysActorCast->KeyValues.Find(TablePropertyName) && *KeysActorCast->KeyValues.Find(TablePropertyName) == PropertyValueName)
							{


								//if (PropertyTypeName == TableTypeName && PropertyValueName == PropertyValueName/*PropertyTypeName == TablePropertyName && ActorPropertyValue.ValueName == PropertyValueName*/)
								//{
								UE_LOG(LogTemp, Warning, TEXT("Draw link KeysActor %s and PropertyActor %s"), *KeysActor->GetName(), *PropertyActor->GetName());
								DrawDebugLine(GetWorld(), KeysActor->GetActorLocation(), PropertyActor->GetActorLocation(), FColor::Green, true, 50.0f, 0, 1.0f);
								//}
							}
						}
					}
				}
			}
		}
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
		FName TypeName = Row->PropertyValue.VariableTypeName;
		FName PropertyName = Row->PropertyName;
		FVariantProperty PropertyValue = Row->PropertyValue;

		if (KeyActor->KeyActorName != TableActorId)
		{
			UE_LOG(LogTemp, Warning, TEXT("Keys Actor %s does not match ID: %s"), *KeyActor->GetName(), *TableActorId.ToString());
			continue;
		}
		KeyActor->ApplyProperty(PropertyName, PropertyValue);
		//KeyActor->TypeName = TypeName;
		KeyActor->KeyTypes.Add(PropertyName, PropertyValue.VariableTypeName);
		KeyActor->KeyValues.Add(PropertyName, PropertyValue.ValueName);

		UE_LOG(LogTemp, Log, TEXT("Applied property %s to Keys Actor: %s"), *PropertyName.ToString(), *KeyActor->GetName());

		for (auto ValueActor : RegisteredPropertyActors)
		{
			APropertyActor* PropertyActor = Cast<APropertyActor>(ValueActor);
			if (!PropertyActor)
			{
				continue;
			}

			if (PropertyActor->Property.VariableTypeName == TypeName && PropertyValue.ValueName == PropertyActor->Property.ValueName)
			{
				DrawDebugLine(GetWorld(), KeyActor->GetActorLocation(), PropertyActor->GetActorLocation(), FColor::Green, true, 50.0f, 0, 1.0f);
			}
		}
	}
}
