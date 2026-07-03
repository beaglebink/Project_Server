#include "SaveSubsystems/GIS_FileTree.h"
#include <GameSaveSubsystem.h>

void UGIS_FileTree::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->RegisterSaveableSubsystem(this);
	}
}

void UGIS_FileTree::Deinitialize()
{
	Super::Deinitialize();
}

void UGIS_FileTree::CollectSaveData(FSubsystemSaveData& OutData)
{
	OutData.SubsystemName = GetSaveSubsystemName();

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	for (UObject* SaveableObject : SaveableObjects)
	{
		if (!IsValid(SaveableObject))
		{
			continue;
		}

		TMap<FString, FCubixonFileData> SaveDataMap = II_SaveableObject::Execute_CollectFileTreeSaveData(SaveableObject);

		TSharedPtr<FJsonObject> ComputerProfile = MakeShared<FJsonObject>();

		for (const auto& Pair : SaveDataMap)
		{
			const FCubixonFileData& FileData = Pair.Value;

			TSharedPtr<FJsonObject> FileJson = MakeShared<FJsonObject>();

			FileJson->SetStringField(TEXT("FileName"), FileData.FileName.ToString());

			FileJson->SetStringField(TEXT("ComponentClass"), FileData.ComponentClass ? FileData.ComponentClass->GetPathName() : TEXT(""));

			FileJson->SetStringField(TEXT("SerializedData"), FBase64::Encode(FileData.SerializedData));

			ComputerProfile->SetObjectField(Pair.Key, FileJson);
		}

		Root->SetObjectField(SaveableObject->GetName(), ComputerProfile);
	}

	FString JsonString;

	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);

	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	OutData.SerializedData = JsonString;
}

void UGIS_FileTree::ApplySaveData(const FSubsystemSaveData& InData)
{
	bIsLoadComplete = false;

	CachedProfilesFileTreeData.Empty();

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

		TMap<FString, FCubixonFileData> SaveDataMap;

		for (const auto& FilePair : (*ProfileJson)->Values)
		{
			const TSharedPtr<FJsonObject>* FileDataJson = nullptr;

			if (!(*ProfileJson)->TryGetObjectField(FilePair.Key, FileDataJson))
			{
				continue;
			}

			FCubixonFileData FileData;

			FString FileNameString;
			if ((*FileDataJson)->TryGetStringField(TEXT("FileName"), FileNameString))
			{
				FileData.FileName = FText::FromString(FileNameString);
			}

			FString ClassPath;
			if ((*FileDataJson)->TryGetStringField(TEXT("ComponentClass"), ClassPath) && !ClassPath.IsEmpty())
			{
				FileData.ComponentClass = LoadClass<USceneComponent>(nullptr, *ClassPath);
			}

			FString EncodedData;
			if ((*FileDataJson)->TryGetStringField(TEXT("SerializedData"), EncodedData))
			{
				FBase64::Decode(EncodedData, FileData.SerializedData);
			}

			SaveDataMap.Add(FilePair.Key, MoveTemp(FileData));
		}

		CachedProfilesFileTreeData.Add(FName(ProfilePair.Key), MoveTemp(SaveDataMap));
	}

	bIsLoadComplete = true;
}

void UGIS_FileTree::ClearTransientData_Implementation()
{
	SaveableObjects.Empty();
	UE_LOG(LogTemp, Log, TEXT("FileTreeSubsystem transient data cleared"));
}

void UGIS_FileTree::AddSaveableObject(UObject* NewSaveableObject)
{
	if (NewSaveableObject && NewSaveableObject->Implements<UI_SaveableObject>())
	{
		SaveableObjects.AddUnique(NewSaveableObject);
	}
}

void UGIS_FileTree::ApplyProfileFileTreeData_Implementation(UObject* ProfileObject)
{
	if (!ProfileObject || !ProfileObject->Implements<UI_SaveableObject>())
	{
		return;
	}

	const FName ProfileName(*ProfileObject->GetName());

	const TMap<FString, FCubixonFileData>* FoundData = CachedProfilesFileTreeData.Find(ProfileName);

	if (!FoundData)
	{
		return;
	}

	II_SaveableObject::Execute_ApplyFileTreeSaveData(ProfileObject, *FoundData);
}
