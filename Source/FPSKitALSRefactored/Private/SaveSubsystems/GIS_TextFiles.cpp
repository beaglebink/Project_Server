#include "SaveSubsystems/GIS_TextFiles.h"
#include <GameSaveSubsystem.h>

void UGIS_TextFiles::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->RegisterSaveableSubsystem(this);
	}
}

void UGIS_TextFiles::Deinitialize()
{
	Super::Deinitialize();
}

void UGIS_TextFiles::CollectSaveData(FSubsystemSaveData& OutData)
{
	OutData.SubsystemName = GetSaveSubsystemName();

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	for (UObject* SaveableObject : SaveableObjects)
	{
		if (!SaveableObject || !SaveableObject->Implements<UI_SaveableObject>())
		{
			continue;
		}

		TMap<FString, FText> SaveDataMap = II_SaveableObject::Execute_CollectTextFilesSaveData(SaveableObject);

		TSharedPtr<FJsonObject> ComputerProfile = MakeShared<FJsonObject>();

		for (const auto& Pair : SaveDataMap)
		{
			ComputerProfile->SetStringField(Pair.Key, Pair.Value.ToString());
		}

		Root->SetObjectField(SaveableObject->GetName(), ComputerProfile);
	}

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	OutData.SerializedData = JsonString;
}

void UGIS_TextFiles::ApplySaveData(const FSubsystemSaveData& InData)
{

	bIsLoadComplete = false;

	CachedProfilesTextFilesData.Empty();

	if (InData.SerializedData.IsEmpty())
	{
		bIsLoadComplete = true;
		return;
	}

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InData.SerializedData);

	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		bIsLoadComplete = true;
		return;
	}

	for (const auto& ProfilePair : Root->Values)
	{
		const TSharedPtr<FJsonObject>* ProfileJson = nullptr;

		if (!Root->TryGetObjectField(ProfilePair.Key, ProfileJson))
		{
			continue;
		}

		TMap<FString, FText> SaveDataMap;

		for (const auto& FilePair : (*ProfileJson)->Values)
		{
			FString Value;

			if (FilePair.Value->TryGetString(Value))
			{
				SaveDataMap.Add(FilePair.Key, FText::FromString(Value));
			}
		}

		CachedProfilesTextFilesData.Add(FName(ProfilePair.Key), MoveTemp(SaveDataMap));
	}

	bIsLoadComplete = true;
}

void UGIS_TextFiles::ClearTransientData_Implementation()
{
	SaveableObjects.Empty();
	UE_LOG(LogTemp, Log, TEXT("TextFilesSubsystem transient data cleared"));
}

void UGIS_TextFiles::AddSaveableObject(UObject* NewSaveableObject)
{
	if (NewSaveableObject && NewSaveableObject->Implements<UI_SaveableObject>())
	{
		SaveableObjects.AddUnique(NewSaveableObject);
	}
}

void UGIS_TextFiles::ApplyProfileTextFilesData_Implementation(UObject* ProfileObject)
{
	if (!ProfileObject || !ProfileObject->Implements<UI_SaveableObject>())
	{
		return;
	}

	const FName ProfileName(*ProfileObject->GetName());

	const TMap<FString, FText>* FoundData = CachedProfilesTextFilesData.Find(ProfileName);

	if (!FoundData)
	{
		return;
	}

	II_SaveableObject::Execute_ApplyTextFilesSaveData(ProfileObject, *FoundData);
}