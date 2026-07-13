#include "SaveSubsystems/GIS_Backup.h"
#include <GameSaveSubsystem.h>

void UGIS_Backup::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->RegisterSaveableSubsystem(this);
	}
}

void UGIS_Backup::Deinitialize()
{
	Super::Deinitialize();
}

void UGIS_Backup::CollectSaveData(FSubsystemSaveData& OutData)
{
	OutData.SubsystemName = GetSaveSubsystemName();

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	for (UObject* SaveableObject : SaveableObjects)
	{
		if (!IsValid(SaveableObject))
		{
			continue;
		}

		FGlobalBackupData SaveData = II_SaveableObject::Execute_CollectBackupSaveData(SaveableObject);

		TSharedPtr<FJsonObject> GlobalBackupDataJson = MakeShared<FJsonObject>();

		// Serialize TextBackupData
		TSharedPtr<FJsonObject> TextFilesBackupJson = MakeShared<FJsonObject>();
		for (const auto& PairFile_Data : SaveData.TextBackupData)
		{
			TSharedPtr<FJsonObject> TextDataBackupJson = MakeShared<FJsonObject>();
			TextDataBackupJson->SetNumberField(TEXT("BackupSize"), PairFile_Data.Value.BackupSize);

			for (const auto& PairTime_Text : PairFile_Data.Value.TimeStamps)
			{
				TextDataBackupJson->SetStringField(PairTime_Text.Key, PairTime_Text.Value.ToString());
			}

			TextFilesBackupJson->SetObjectField(PairFile_Data.Key, TextDataBackupJson);
		}
		GlobalBackupDataJson->SetObjectField(TEXT("TextFilesBackupData"), TextFilesBackupJson);

		// Serialize SpreadsheetBackupData
		TSharedPtr<FJsonObject> SpreadsheetFilesBackupJson = MakeShared<FJsonObject>();
		for (const auto& PairFile_Data : SaveData.SheetBackupData)
		{
			TSharedPtr<FJsonObject> SpreadsheetDataBackupJson = MakeShared<FJsonObject>();
			SpreadsheetDataBackupJson->SetNumberField(TEXT("BackupSize"), PairFile_Data.Value.BackupSize);

			for (const auto& PairTime_Cell : PairFile_Data.Value.TimeStamps)
			{
				TSharedPtr<FJsonObject> SpreadsheetCellsJson = MakeShared<FJsonObject>();
				for (const auto& PairCoord_Text : PairTime_Cell.Value.Cells)
				{
					SpreadsheetCellsJson->SetStringField(FString::Printf(TEXT("%d,%d"), PairCoord_Text.Key.X, PairCoord_Text.Key.Y), PairCoord_Text.Value.ToString());
				}
				SpreadsheetDataBackupJson->SetObjectField(PairTime_Cell.Key, SpreadsheetCellsJson);
			}

			SpreadsheetFilesBackupJson->SetObjectField(PairFile_Data.Key, SpreadsheetDataBackupJson);
		}
		GlobalBackupDataJson->SetObjectField(TEXT("SpreadsheetFilesBackupData"), SpreadsheetFilesBackupJson);

		// Serialize ScheduledBackupData
		TSharedPtr<FJsonObject> ScheduledFilesBackupJson = MakeShared<FJsonObject>();
		for (const auto& PairFile_Data : SaveData.ScheduledBackupData)
		{
			TSharedPtr<FJsonObject> ScheduledBackupDataJson = MakeShared<FJsonObject>();
			ScheduledBackupDataJson->SetNumberField(TEXT("TimePeriod"), PairFile_Data.Value.TimePeriod);
			ScheduledBackupDataJson->SetStringField(TEXT("NextTimeAutoBackup"), PairFile_Data.Value.NextTimeAutoBackup.ToString());

			ScheduledFilesBackupJson->SetObjectField(PairFile_Data.Key, ScheduledBackupDataJson);
		}
		GlobalBackupDataJson->SetObjectField(TEXT("ScheduledBackupData"), ScheduledFilesBackupJson);

		Root->SetObjectField(SaveableObject->GetName(), GlobalBackupDataJson);
	}

	FString JsonString;

	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);

	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	OutData.SerializedData = JsonString;
}

void UGIS_Backup::ApplySaveData(const FSubsystemSaveData& InData)
{
	bIsLoadComplete = false;

	CachedBackupData.Empty();

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
		const TSharedPtr<FJsonObject>* GlobalBackupDataJson = nullptr;

		if (!Root->TryGetObjectField(ProfilePair.Key, GlobalBackupDataJson))
		{
			continue;
		}

		FGlobalBackupData GlobalBackupData;

		// Deserialize TextBackupData
		const TSharedPtr<FJsonObject>* TextFilesBackupJson;
		if (!(*GlobalBackupDataJson)->TryGetObjectField(TEXT("TextFilesBackupData"), TextFilesBackupJson))
		{
			continue;
		}

		for (const auto& PairFile_Data : (*TextFilesBackupJson)->Values)
		{
			FTextBackupTimeStamp TextBackupTimeStamp;

			TSharedPtr<FJsonObject> TextDataBackupJson = PairFile_Data.Value->AsObject();

			if (!TextDataBackupJson.IsValid())
			{
				continue;
			}

			double BackupSize;
			if (TextDataBackupJson->TryGetNumberField(TEXT("BackupSize"), BackupSize))
			{
				TextBackupTimeStamp.BackupSize = static_cast<float>(BackupSize);
			}

			for (const auto& PairTime_Text : TextDataBackupJson->Values)
			{
				if (PairTime_Text.Key == TEXT("BackupSize"))
				{
					continue;
				}

				FString TextValue;
				if (PairTime_Text.Value->TryGetString(TextValue))
				{
					TextBackupTimeStamp.TimeStamps.Add(PairTime_Text.Key, FText::FromString(TextValue));
				}
			}

			GlobalBackupData.TextBackupData.Add(PairFile_Data.Key, TextBackupTimeStamp);
		}

		// Deserialize SpreadsheetBackupData
		const TSharedPtr<FJsonObject>* SpreadsheetFilesBackupJson;
		if (!(*GlobalBackupDataJson)->TryGetObjectField(TEXT("SpreadsheetFilesBackupData"), SpreadsheetFilesBackupJson))
		{
			continue;
		}

		for (const auto& PairFile_Data : (*SpreadsheetFilesBackupJson)->Values)
		{
			FSheetBackupTimeStamp SheetBackupTimeStamp;

			TSharedPtr<FJsonObject> SheetDataBackupJson = PairFile_Data.Value->AsObject();

			if (!SheetDataBackupJson.IsValid())
			{
				continue;
			}

			double BackupSize;
			if (SheetDataBackupJson->TryGetNumberField(TEXT("BackupSize"), BackupSize))
			{
				SheetBackupTimeStamp.BackupSize = static_cast<float>(BackupSize);
			}

			for (const auto& PairTime_Sheet : SheetDataBackupJson->Values)
			{
				if (PairTime_Sheet.Key == TEXT("BackupSize"))
				{
					continue;
				}

				FSpreadsheetCells Cells;
				TSharedPtr<FJsonObject> SheetCellsJson = PairTime_Sheet.Value->AsObject();
				for (const auto& PairCoord_Text : SheetCellsJson->Values)
				{
					FIntPoint CellKey;
					FString XString;
					FString YString;

					if (PairCoord_Text.Key.Split(TEXT(","), &XString, &YString))
					{
						CellKey.X = FCString::Atoi(*XString);
						CellKey.Y = FCString::Atoi(*YString);

						FString TextValue;
						if (PairCoord_Text.Value->TryGetString(TextValue))
						{
							Cells.Cells.Add(CellKey, FText::FromString(TextValue));
						}
					}
				}

				SheetBackupTimeStamp.TimeStamps.Add(PairTime_Sheet.Key, Cells);
			}

			GlobalBackupData.SheetBackupData.Add(PairFile_Data.Key, SheetBackupTimeStamp);
		}

		// Deserialize ScheduledBackupData
		const TSharedPtr<FJsonObject>* ScheduledFilesBackupJson;
		if (!(*GlobalBackupDataJson)->TryGetObjectField(TEXT("ScheduledBackupData"), ScheduledFilesBackupJson))
		{
			continue;
		}

		for (const auto& PairFile_Data : (*ScheduledFilesBackupJson)->Values)
		{
			FScheduledBackupData ScheduledBackupData;

			TSharedPtr<FJsonObject> ScheduledBackupDataJson = PairFile_Data.Value->AsObject();

			if (!ScheduledBackupDataJson.IsValid())
			{
				continue;
			}

			double TimePeriod;
			if (ScheduledBackupDataJson->TryGetNumberField(TEXT("TimePeriod"), TimePeriod))
			{
				ScheduledBackupData.TimePeriod = static_cast<int32>(TimePeriod);
			}

			FString NextTimeAutoBackupString;
			if (ScheduledBackupDataJson->TryGetStringField(TEXT("NextTimeAutoBackup"), NextTimeAutoBackupString))
			{
				FDateTime::Parse(NextTimeAutoBackupString, ScheduledBackupData.NextTimeAutoBackup);
			}

			GlobalBackupData.ScheduledBackupData.Add(PairFile_Data.Key, ScheduledBackupData);
		}

		CachedBackupData.Add(FName(ProfilePair.Key), MoveTemp(GlobalBackupData));
	}

	bIsLoadComplete = true;
}

void UGIS_Backup::ClearTransientData_Implementation()
{
	SaveableObjects.Empty();
	UE_LOG(LogTemp, Log, TEXT("BackupSubsystem transient data cleared"));
}

void UGIS_Backup::AddSaveableObject(UObject* NewSaveableObject)
{
	if (NewSaveableObject && NewSaveableObject->Implements<UI_SaveableObject>())
	{
		SaveableObjects.AddUnique(NewSaveableObject);
	}
}

void UGIS_Backup::ApplyBackupData_Implementation(UObject* ProfileObject)
{
	if (!ProfileObject || !ProfileObject->Implements<UI_SaveableObject>())
	{
		return;
	}

	const FName ProfileName(*ProfileObject->GetName());

	const FGlobalBackupData* FoundData = CachedBackupData.Find(ProfileName);

	if (!FoundData)
	{
		return;
	}

	II_SaveableObject::Execute_ApplyBackupSaveData(ProfileObject, *FoundData);
}

