#include "SaveSubsystems/GIS_TextFiles.h"
#include <GameSaveSubsystem.h>

void UGIS_TextFiles::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("TextFilesSubsystem initialized"));

	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->RegisterSaveableSubsystem(this);
	}
}

void UGIS_TextFiles::Deinitialize()
{
	Super::Deinitialize();

	UE_LOG(LogTemp, Log, TEXT("TextFilesSubsystem deinitialized"));
}

void UGIS_TextFiles::CollectSaveData(FSubsystemSaveData& OutData)
{
	OutData.SubsystemName = GetSaveSubsystemName();

	TMap<FString, FText> SaveDataMap;
	if (SaveableObject && SaveableObject->Implements<UI_SaveableObject>())
	{
		SaveDataMap = II_SaveableObject::Execute_CollectTextFilesSaveData(SaveableObject);
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	for (const auto& Pair : SaveDataMap)
	{
		Root->SetStringField(Pair.Key, Pair.Value.ToString());
	}

	FString JsonString;

	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);

	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	OutData.SerializedData = JsonString;
}

void UGIS_TextFiles::ApplySaveData(const FSubsystemSaveData& InData)
{
	bIsLoadComplete = false;

	if (InData.SerializedData.IsEmpty() || !SaveableObject || !SaveableObject->Implements<UI_SaveableObject>())
	{
		bIsLoadComplete = true;
		return;
	}

	TSharedPtr<FJsonObject> Root;

	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InData.SerializedData);

	if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
	{
		TMap<FString, FText> SaveDataMap;

		for (const auto& Pair : Root->Values)
		{
			FString Value;

			if (Pair.Value->TryGetString(Value))
			{
				SaveDataMap.Add(Pair.Key, FText::FromString(Value));
			}
		}

		II_SaveableObject::Execute_ApplyTextFilesSaveData(SaveableObject, SaveDataMap);
	}
};
