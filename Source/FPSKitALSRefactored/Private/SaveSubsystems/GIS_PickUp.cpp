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

	//for (UObject* SaveableObject : SaveableObjects)
	//{
	//	if (!SaveableObject || !SaveableObject->Implements<UI_SaveableObject>())
	//	{
	//		continue;
	//	}

	//	FFlyerPostingSaveData FlyerPostingSaveData = II_SaveableObject::Execute_CollectFlyerPostingSaveData(SaveableObject);

	//	TSharedPtr<FJsonObject> FlyerPostingSaveDataJsonObject = MakeShared<FJsonObject>();

	//	FlyerPostingSaveDataJsonObject->SetNumberField(TEXT("ChoreStatus"), static_cast<int32>(FlyerPostingSaveData.ChoreStatus));

	//	TSharedPtr<FJsonObject> FlyersSaveDataJsonObject = MakeShared<FJsonObject>();
	//	for (const auto& [FlyerId, FlyerTransform] : FlyerPostingSaveData.PlacedFlyers)
	//	{
	//		FlyersSaveDataJsonObject->SetStringField(FlyerId.ToString(), FlyerTransform.ToString());
	//	}
	//	FlyerPostingSaveDataJsonObject->SetObjectField(TEXT("FlyersData"), FlyersSaveDataJsonObject);

	//	FString FlyerPostingObjectName = II_SaveableObject::Execute_GetFloorAssignmentComponentIdAsString(SaveableObject);
	//	Root->SetObjectField(FlyerPostingObjectName, FlyerPostingSaveDataJsonObject);
	//}

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

	for (const auto& [FlyerPostingObjectId, FlyerPostingSaveDataJsonValue] : Root->Values)
	{
		FFlyerPostingSaveData FlyerPostingSaveData;

		TSharedPtr<FJsonObject> FlyerPostingSaveDataJsonObject = FlyerPostingSaveDataJsonValue->AsObject();

		if (!FlyerPostingSaveDataJsonObject)
		{
			continue;
		}

		int32 FlyerPostingStatusNumbered;
		if (!FlyerPostingSaveDataJsonObject->TryGetNumberField(TEXT("ChoreStatus"), FlyerPostingStatusNumbered))
		{
			continue;
		}
		FlyerPostingSaveData.ChoreStatus = static_cast<EChoreStatus>(FlyerPostingStatusNumbered);

		const TSharedPtr<FJsonObject>* FlyersSaveDataJsonObject = nullptr;
		if (!FlyerPostingSaveDataJsonObject->TryGetObjectField(TEXT("FlyersData"), FlyersSaveDataJsonObject))
		{
			continue;
		}

		TMap<FGuid, FTransform> PlacedFlyersData;
		for (const auto& [FlyerIdString, FlyerTransformJsonValue] : (*FlyersSaveDataJsonObject)->Values)
		{
			FGuid FlyerId = FGuid(FlyerIdString);

			FString FlyerTransformString;
			if (!FlyerTransformJsonValue->TryGetString(FlyerTransformString))
			{
				continue;
			}
			FTransform FlyerTransform;
			if (!FlyerTransform.InitFromString(FlyerTransformString))
			{
				continue;
			}
			PlacedFlyersData.Add(FlyerId, FlyerTransform);
		}
		FlyerPostingSaveData.PlacedFlyers = MoveTemp(PlacedFlyersData);

		//CachedPickUpData.Add(FlyerPostingObjectId, FlyerPostingSaveData);
	}

	bIsLoadComplete = true;
}

void UGIS_PickUp::ClearTransientData_Implementation()
{
	SaveableObjects.Empty();
	UE_LOG(LogTemp, Log, TEXT("FlyerPosting transient data cleared"));
}

void UGIS_PickUp::AddSaveableObject(UObject* NewSaveableObject)
{
	if (NewSaveableObject && NewSaveableObject->Implements<UI_SaveableObject>())
	{
		SaveableObjects.AddUnique(NewSaveableObject);
	}
}