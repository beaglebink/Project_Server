#include "SaveSubsystems/GIS_PickUp.h"
#include <GameSaveSubsystem.h>
#include "FlyerPosting/A_FlyerPostingBase.h"

void UGIS_PickUp::Initialize(FSubsystemCollectionBase& Collection)
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

void UGIS_PickUp::Deinitialize()
{
	// Отписка от сохранения
	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->UnregisterSaveableSubsystem(this);
	}
	Super::Deinitialize();
}

void UGIS_PickUp::CollectSaveData(FSubsystemSaveData& OutData)
{
	OutData.SubsystemName = GetSaveSubsystemName();

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	for (UObject* SaveableObject : SaveableObjects)
	{
		if (!SaveableObject || !SaveableObject->Implements<UI_SaveableObject>())
		{
			continue;
		}

		FPickUpSaveData PickUpSaveData = II_SaveableObject::Execute_CollectPickUpSaveData(SaveableObject);

		TSharedPtr<FJsonObject> PickUpSaveDataJsonObject = MakeShared<FJsonObject>();

		PickUpSaveDataJsonObject->SetStringField(TEXT("PickUpTransform"), PickUpSaveData.PickUpTransform.ToString());

		FString PickUpObjectName = II_SaveableObject::Execute_GetFloorAssignmentComponentIdAsString(SaveableObject);
		Root->SetObjectField(PickUpObjectName, PickUpSaveDataJsonObject);
	}

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	OutData.SerializedData = JsonString;
}

void UGIS_PickUp::ApplySaveData(const FSubsystemSaveData& InData)
{

	bIsLoadComplete = false;

	CachedPickUpData.Empty();

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

	for (const auto& [PickUpObjectIdString, PickUpSaveDataJsonValue] : Root->Values)
	{
		FPickUpSaveData PickUpSaveData;

		TSharedPtr<FJsonObject> PickUpSaveDataJsonObject = PickUpSaveDataJsonValue->AsObject();

		if (!PickUpSaveDataJsonObject)
		{
			continue;
		}

		FString PickUpTransformString;
		if (!PickUpSaveDataJsonObject->TryGetStringField(TEXT("PickUpTransform"), PickUpTransformString))
		{
			continue;
		}
		FTransform PickUpTransform;
		if (!PickUpTransform.InitFromString(PickUpTransformString))
		{
			continue;
		}
		PickUpSaveData.PickUpTransform = PickUpTransform;
			
			
		CachedPickUpData.Add(PickUpObjectIdString, PickUpSaveData);
	}

	bIsLoadComplete = true;
}

void UGIS_PickUp::ClearTransientData_Implementation()
{
	SaveableObjects.Empty();
	UE_LOG(LogTemp, Log, TEXT("PickUp transient data cleared"));
}

void UGIS_PickUp::AddSaveableObject(UObject* NewSaveableObject)
{
	if (NewSaveableObject && NewSaveableObject->Implements<UI_SaveableObject>())
	{
		SaveableObjects.AddUnique(NewSaveableObject);
	}
}