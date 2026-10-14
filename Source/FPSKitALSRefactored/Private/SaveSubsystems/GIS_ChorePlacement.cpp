#include "SaveSubsystems/GIS_ChorePlacement.h"
#include <GameSaveSubsystem.h>

void UGIS_ChorePlacement::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Форсируем инициализацию системы сохранения ДО нас.
	// Без этого GetSubsystem<UGameSaveSubsystem>() может вернуть nullptr,
	// и регистрация Saveable-подсистемы молча не сработает.
	Collection.InitializeDependency<UGameSaveSubsystem>();

	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->RegisterSaveableSubsystem(this);
	}
}

void UGIS_ChorePlacement::Deinitialize()
{
	// Отписка от сохранения
	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->UnregisterSaveableSubsystem(this);
	}
	Super::Deinitialize();
}

void UGIS_ChorePlacement::CollectSaveData(FSubsystemSaveData& OutData)
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

void UGIS_ChorePlacement::ApplySaveData(const FSubsystemSaveData& InData)
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

void UGIS_ChorePlacement::ClearTransientData_Implementation()
{
	SaveableObjects.Empty();
	UE_LOG(LogTemp, Log, TEXT("ChorePlacementSubsystem transient data cleared"));
}

void UGIS_ChorePlacement::AddSaveableObject(UObject* NewSaveableObject)
{
	if (NewSaveableObject && NewSaveableObject->Implements<UI_SaveableObject>())
	{
		SaveableObjects.AddUnique(NewSaveableObject);
	}
}

void UGIS_ChorePlacement::ApplyChorePlacementData_Implementation(UObject* ChorePlacementObject)
{
	if (!ChorePlacementObject || !ChorePlacementObject->Implements<UI_SaveableObject>())
	{
		return;
	}

	const FName ProfileName(*ChorePlacementObject->GetName());

	const TMap<FString, FText>* FoundData = CachedProfilesTextFilesData.Find(ProfileName);

	if (!FoundData)
	{
		return;
	}

	//II_SaveableObject::Execute_ApplyTextFilesSaveData(ProfileObject, *FoundData);
}