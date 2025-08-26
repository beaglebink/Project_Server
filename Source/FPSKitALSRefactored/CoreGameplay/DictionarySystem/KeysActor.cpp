#include "KeysActor.h"
#include "DictionaryManager.h"
#include <Kismet/KismetStringLibrary.h>

void AKeysActor::BeginPlay()
{
	Super::BeginPlay();
	if (ManagerInstance)
	{
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
		ManagerInstance->UnregisterKeysActor(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("KeysActor: ManagerInstance is null during EndPlay!"));
	}
}
void AKeysActor::AddPropertyDescription(const FString& PropertyName, const FString& ValueName)
{
	
}

TArray<FString> AKeysActor::ParseCommands(const FText& InputText) const
{
	TArray<FString> Commands;

	Commands = UKismetStringLibrary::ParseIntoArray(InputText.ToString(), TEXT("\n"), true);

	return Commands;
}

void AKeysActor::ParseCommand(const FString& Command, FString& Type, FString& Key, FString& Value) const
{  
   FString TrimmedCommand = Command.TrimStartAndEnd();  
   FString Left, Right;  


   if (TrimmedCommand.Split(TEXT(":"), &Left, &Right))  
   {  
       Key = *Left.TrimStartAndEnd();  

	   if (TrimmedCommand.Split(TEXT("="), &Left, &Right))
	   {
		   Type = TrimmedCommand.Mid(TrimmedCommand.Find(TEXT(":")) + 1, TrimmedCommand.Find(TEXT("=")) - TrimmedCommand.Find(TEXT(":")) - 1).TrimStartAndEnd();
		   Value = *Right.TrimStartAndEnd();
	   }
	   else
	   {
		   Type = FString();
		   Key = *TrimmedCommand;
		   Value = FString();
	   }
   }  
   else if (TrimmedCommand.Split(TEXT("="), &Left, &Right))  
   {  
	   Type = FString();
       Key = *Left.TrimStartAndEnd();  
       Value = *Right.TrimStartAndEnd();
   }  
   else  
   {  
	   Type = FString();
       Key = *TrimmedCommand;  
       Value = FString();
   }  
}

void AKeysActor::ParseText(const FText Text)
{
	
	TArray<FString> Commands = ParseCommands(Text);
	for (auto Command : Commands)
	{
		FString Type, Key, Value;
		ParseCommand(Command, Type, Key, Value);
		if (!Key.IsEmpty())
		{
			if (!Value.IsEmpty())
			{
				if (FString* T = FindStrict(KeyTypes, Key))
				{
					if (FString* T1 = FindStrict(KeyValues, Key))
					{
						if (!KeyTypes.Contains(Key) || !KeyValues.Contains(Key))
						{
							UE_LOG(LogTemp, Log, TEXT("No property: %s"), *Key);
							continue;
						}

						FString MapValue;
						if (!Type.IsEmpty())
						{
							FString MapType = *KeyTypes.Find(Key);
							if (UKismetStringLibrary::EqualEqual_StrStr(MapType, Type)/*MapType == Type*/)
							{
								MapValue = *KeyValues.Find(Key);
							}
						}
						else
						{
							Type = *KeyTypes.Find(Key);
							MapValue = *KeyValues.Find(Key);
						}

						if (!MapValue.IsEmpty() && MapValue != Value)
						{
							UE_LOG(LogTemp, Warning, TEXT("Key %s already has a different value: %s. Overwriting with new value: %s"), *Key, *MapValue, *Value);

							FVariantProperty VarProperty;

							AActor* OldActor = ManagerInstance->VerifyProperty(Type, MapValue, VarProperty);
							AActor* PropertyActor = ManagerInstance->VerifyProperty(Type, Value, VarProperty);

							if (PropertyActor)
							{
								KeyValues.Add(Key, Value);
								ApplyProperty(Key, VarProperty);
								ManagerInstance->ConnectActorChain(this, Key, PropertyActor, OldActor);
							}
							else
							{
								UE_LOG(LogTemp, Warning, TEXT("Property value %s not found in DictionaryManager."), *Value);
							}
						}
					}
				}
			}
		}
	}
}

void AKeysActor::ApplyProperty_Implementation(const FString& PropertyName, const FVariantProperty& Value)
{
}

FString* AKeysActor::FindStrict(TMap<FString, FString>& Map, const FString& Key)
{
	for (auto& Pair : Map)
	{
		if (Pair.Key.Equals(Key, ESearchCase::CaseSensitive))
		{
			return &Pair.Value;
		}
	}
	return nullptr;
}
