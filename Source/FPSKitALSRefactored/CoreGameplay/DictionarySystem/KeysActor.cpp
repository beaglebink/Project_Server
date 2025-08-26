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
void AKeysActor::AddPropertyDescription(const FName PropertyName, const FName ValueName)
{
	
}

TArray<FString> AKeysActor::ParseCommands(const FText& InputText) const
{
	TArray<FString> Commands;

	Commands = UKismetStringLibrary::ParseIntoArray(InputText.ToString(), TEXT("\n"), true);

	return Commands;
}

void AKeysActor::ParseCommand(const FString& Command, FName& Type, FName& Key, FName& Value) const  
{  
   FString TrimmedCommand = Command.TrimStartAndEnd();  
   FString Left, Right;  


   if (TrimmedCommand.Split(TEXT(":"), &Left, &Right))  
   {  
       Key = FName(*Left.TrimStartAndEnd());  

	   if (TrimmedCommand.Split(TEXT("="), &Left, &Right))
	   {
		   Type = FName(TrimmedCommand.Mid(TrimmedCommand.Find(TEXT(":")) + 1, TrimmedCommand.Find(TEXT("=")) - TrimmedCommand.Find(TEXT(":")) - 1).TrimStartAndEnd());
		   Value = FName(*Right.TrimStartAndEnd());
	   }
	   else
	   {
		   Type = FName();
		   Key = FName(*TrimmedCommand);
		   Value = FName();
	   }
   }  
   else if (TrimmedCommand.Split(TEXT("="), &Left, &Right))  
   {  
	   Type = FName();
       Key = FName(*Left.TrimStartAndEnd());  
       Value = FName(*Right.TrimStartAndEnd());  
   }  
   else  
   {  
	   Type = FName();
       Key = FName(*TrimmedCommand);  
       Value = FName();  
   }  
}

void AKeysActor::ParseText(const FText Text)
{
	
	TArray<FString> Commands = ParseCommands(Text);
	for (auto Command : Commands)
	{
		FName Type, Key, Value;
		ParseCommand(Command, Type, Key, Value);
		if (Key != NAME_None)
		{
			if (Value != NAME_None)
			{
				if (!KeyTypes.Contains(Key) || !KeyValues.Contains(Key))
				{
					UE_LOG(LogTemp, Log, TEXT("No property: %s"), *Key.ToString());
					continue;
				}

				FName MapValue;
				if (!Type.IsNone())
				{
					FName MapType = *KeyTypes.Find(Key);
					if (MapType == Type)
					{
						MapValue = *KeyValues.Find(Key);
					}
				}
				else
				{
					Type = *KeyTypes.Find(Key);
					MapValue = *KeyValues.Find(Key);
				}

				if (!MapValue.IsNone() && MapValue != Value)
				{
					UE_LOG(LogTemp, Warning, TEXT("Key %s already has a different value: %s. Overwriting with new value: %s"), *Key.ToString(), *MapValue.ToString(), *Value.ToString());

					FVariantProperty VarProperty;

					AActor* OldActor = ManagerInstance->VerifyProperty(Type, MapValue, VarProperty);
					AActor* PropertyActor = ManagerInstance->VerifyProperty(Type, Value, VarProperty);

					if (PropertyActor)
					{
						KeyValues.Add(Key, Value);
						ApplyProperty(Key, VarProperty);
						ManagerInstance->ConnectActorChain(this, PropertyActor, OldActor);
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("Property value %s not found in DictionaryManager."), *Value.ToString());
					}
				}


				

				
			}
		}
	}
}

void AKeysActor::ApplyProperty_Implementation(const FName PropertyName, const FVariantProperty Value)
{
}

