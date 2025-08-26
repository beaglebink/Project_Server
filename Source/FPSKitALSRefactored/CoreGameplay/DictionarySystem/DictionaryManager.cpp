#include "DictionaryManager.h"
#include <FPSKitALSRefactored/CoreGameplay/TeleportationSystem/SceneDataProvider.h>
#include "PropertyActor.h"
#include <NiagaraFunctionLibrary.h>
#include "NiagaraComponent.h"



ADictionaryManager::ADictionaryManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

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

	ConnectionEffect = ISceneDataProvider::Execute_ConnectionEffect(GI);

	if (ConnectionEffect)
	{
		UE_LOG(LogTemp, Log, TEXT("ConnectionEffect loaded: %s"), *ConnectionEffect->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load ConnectionEffect"));
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
		FString PropertyTypeName = ActorPropertyValue.VariableTypeName;
		FString PropertyValueName = ActorPropertyValue.ValueName;

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
					continue;
				}

				FName TableActorId = Row->ActorID;
				FString TableTypeName = Row->PropertyValue.VariableTypeName;
				FString TablePropertyName = Row->PropertyName;
				FVariantProperty TablePropertyValue = Row->PropertyValue;
				FString TablePropertyValueName = TablePropertyValue.ValueName;

				if (KeysActorCast->KeyTypes.Contains(TablePropertyName))
				{
					if (KeysActorCast->KeyTypes.Find(TablePropertyName) && *KeysActorCast->KeyTypes.Find(TablePropertyName) == PropertyTypeName)
					{
						if (KeysActorCast->KeyValues.Contains(TablePropertyName))
						{
							if (KeysActorCast->KeyValues.Find(TablePropertyName) && *KeysActorCast->KeyValues.Find(TablePropertyName) == PropertyValueName)
							{
								UE_LOG(LogTemp, Warning, TEXT("Draw link KeysActor %s and PropertyActor %s"), *KeysActor->GetName(), *PropertyActor->GetName());

								ConnectActorChain(KeysActor, TablePropertyName, PropertyActor, nullptr);
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
		FString TypeName = Row->PropertyValue.VariableTypeName;
		FString PropertyName = Row->PropertyName;
		FVariantProperty PropertyValue = Row->PropertyValue;

		if (KeyActor->KeyActorName != TableActorId)
		{
			UE_LOG(LogTemp, Warning, TEXT("Keys Actor %s does not match ID: %s"), *KeyActor->GetName(), *TableActorId.ToString());
			continue;
		}
		KeyActor->ApplyProperty(PropertyName, PropertyValue);
		KeyActor->KeyTypes.Add(PropertyName, PropertyValue.VariableTypeName);
		KeyActor->KeyValues.Add(PropertyName, PropertyValue.ValueName);

		UE_LOG(LogTemp, Log, TEXT("Applied property %s to Keys Actor: %s"), *PropertyName, *KeyActor->GetName());

		for (auto ValueActor : RegisteredPropertyActors)
		{
			APropertyActor* PropertyActor = Cast<APropertyActor>(ValueActor);
			if (!PropertyActor)
			{
				continue;
			}

			if (PropertyActor->Property.VariableTypeName == TypeName && PropertyValue.ValueName == PropertyActor->Property.ValueName)
			{
				ConnectActorChain(KeyActor, PropertyName, PropertyActor, nullptr);
			}
		}
	}
}
AActor* ADictionaryManager::VerifyProperty(const FString& PropertyType, const FString& PropertyValue, FVariantProperty& VariantProperty)
{
	for (auto ValueActor : RegisteredPropertyActors)
	{
		if (ValueActor->Property.VariableTypeName == PropertyType && ValueActor->Property.ValueName == PropertyValue)
		{
			VariantProperty = ValueActor->Property;

			return ValueActor;
		}
	}
	return nullptr;
}

void ADictionaryManager::ConnectActorChain(AActor* Start, const FString& Key, AActor* Finish, AActor* Old)
{
	FEffectsStruct EStruct;
	EStruct.KeyActor = Start;
	EStruct.KeyName = Key;
	EStruct.PropertyActor = Old;

	if (ActiveEffects.Find(EStruct))
	{
		auto Effect = ActiveEffects.FindRef(EStruct);

		ActiveEffects.Remove(EStruct);

		FVector StartPoint = Start->GetActorLocation();
		FVector EndPoint = Finish->GetActorLocation();

		Effect->SetVectorParameter(FName("Beam Start"), StartPoint);
		Effect->SetVectorParameter(FName("Beam End"), EndPoint);

		FEffectsStruct EStruct_New;
		EStruct_New.KeyActor = Start;
		EStruct_New.KeyName = Key;
		EStruct_New.PropertyActor = Finish;

		ActiveEffects.Remove(EStruct);
		ActiveEffects.Add(EStruct_New, Effect);

		return;

	}

	if (ConnectionEffect)
	{
		FVector StartPoint = Start->GetActorLocation();
		FVector EndPoint = Finish->GetActorLocation();

		UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(ConnectionEffect, Start->GetRootComponent(), FName(TEXT("Niagara")), FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true, true);
		if (NiagaraComp)
		{
			NiagaraComp->SetVectorParameter(FName("Beam Start"), StartPoint);
			NiagaraComp->SetVectorParameter(FName("Beam End"), EndPoint);
			NiagaraComp->SetAutoDestroy(true);

			FEffectsStruct EStruct_New;
			EStruct_New.KeyActor = Start;
			EStruct_New.KeyName = Key;
			EStruct_New.PropertyActor = Finish;

			ActiveEffects.Add(EStruct_New, NiagaraComp);
		}
	}
}
