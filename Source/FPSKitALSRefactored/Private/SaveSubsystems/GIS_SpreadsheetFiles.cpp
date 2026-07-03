#include "SaveSubsystems/GIS_SpreadsheetFiles.h"
#include <GameSaveSubsystem.h>

void UGIS_SpreadsheetFiles::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->RegisterSaveableSubsystem(this);
	}
}

void UGIS_SpreadsheetFiles::Deinitialize()
{
	Super::Deinitialize();
}

void UGIS_SpreadsheetFiles::CollectSaveData(FSubsystemSaveData& OutData)
{
	OutData.SubsystemName = GetSaveSubsystemName();

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	for (UObject* SaveableObject : SaveableObjects)
	{
		TMap<FString, FSpreadsheetCells> SaveDataMap = II_SaveableObject::Execute_CollectSpreadsheetSaveData(SaveableObject);

		TSharedPtr<FJsonObject> ComputerProfile = MakeShared<FJsonObject>();

		for (const auto& Pair : SaveDataMap)
		{
			TSharedPtr<FJsonObject> SpreadsheetCells = MakeShared<FJsonObject>();

			for (const auto& CellPair : Pair.Value.Cells)
			{
				SpreadsheetCells->SetStringField(FString::Printf(TEXT("%d,%d"), CellPair.Key.X, CellPair.Key.Y), CellPair.Value.ToString());
			}

			ComputerProfile->SetObjectField(Pair.Key, SpreadsheetCells);
		}

		Root->SetObjectField(SaveableObject->GetName(), ComputerProfile);
	}

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	OutData.SerializedData = JsonString;
}

void UGIS_SpreadsheetFiles::ApplySaveData(const FSubsystemSaveData& InData)
{

	bIsLoadComplete = false;

	CachedProfilesSpreadsheetFilesData.Empty();

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

		TMap<FString, FSpreadsheetCells> SaveDataMap;

		for (const auto& FilePair : (*ProfileJson)->Values)
		{
			const TSharedPtr<FJsonObject>* SpreadsheetCellsJson = nullptr;

			if (!(*ProfileJson)->TryGetObjectField(FilePair.Key, SpreadsheetCellsJson))
			{
				continue;
			}

			FSpreadsheetCells SpreadsheetCells;

			for (const auto& CellPair : (*SpreadsheetCellsJson)->Values)
			{
				FIntPoint CellKey;
				FString XString;
				FString YString;

				if (CellPair.Key.Split(TEXT(","), &XString, &YString))
				{
					CellKey.X = FCString::Atoi(*XString);
					CellKey.Y = FCString::Atoi(*YString);
					FString CellValue;
					if (CellPair.Value->TryGetString(CellValue))
					{
						SpreadsheetCells.Cells.Add(CellKey, FText::FromString(CellValue));
					}
				}
			}
			SaveDataMap.Add(FilePair.Key, SpreadsheetCells);

		}

		CachedProfilesSpreadsheetFilesData.Add(FName(ProfilePair.Key), MoveTemp(SaveDataMap));
	}

	bIsLoadComplete = true;
}

void UGIS_SpreadsheetFiles::ClearTransientData_Implementation()
{
	SaveableObjects.Empty();
	UE_LOG(LogTemp, Log, TEXT("SpreadsheetFilesSubsystem transient data cleared"));
}

void UGIS_SpreadsheetFiles::AddSaveableObject(UObject* NewSaveableObject)
{
	if (NewSaveableObject && NewSaveableObject->Implements<UI_SaveableObject>())
	{
		SaveableObjects.AddUnique(NewSaveableObject);
	}
}

void UGIS_SpreadsheetFiles::ApplyProfileSpreadsheetFilesData_Implementation(UObject* ProfileObject)
{
	if (!ProfileObject || !ProfileObject->Implements<UI_SaveableObject>())
	{
		return;
	}

	const FName ProfileName(*ProfileObject->GetName());

	const TMap<FString, FSpreadsheetCells>* FoundData = CachedProfilesSpreadsheetFilesData.Find(ProfileName);

	if (!FoundData)
	{
		return;
	}

	II_SaveableObject::Execute_ApplySpreadsheetSaveData(ProfileObject, *FoundData);
}

