#include "SaveSubsystems/GIS_ChorePlacement.h"
#include <GameSaveSubsystem.h>
#include "FloorAssignmentComponent.h"

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

		FSaveChoreData SaveChoreData = II_SaveableObject::Execute_CollectChorePlacementSaveData(SaveableObject);

		FGuid SaveableObjectID;
		if (AA_ChorePlacementZone* ChorePlacementZone = Cast<AA_ChorePlacementZone>(SaveableObject))
		{
			SaveableObjectID = ChorePlacementZone->FloorAssignmentComponent->GetItemId();
		}
		if (!SaveableObjectID.IsValid())
		{
			continue;
		}

		FString ZoneKey = SaveableObjectID.ToString();

		TArray<TSharedPtr<FJsonValue>> ItemsJsonArray;

		for (const FSaveItemData& SaveItemData : SaveChoreData.SaveItemData)
		{
			TSharedPtr<FJsonObject> ItemJson = MakeShared<FJsonObject>();

			ItemJson->SetStringField(TEXT("ItemId"), SaveItemData.ItemId.ToString());
			ItemJson->SetStringField(TEXT("AttachingDropZoneId"), SaveItemData.AttachingDropZoneId.ToString());
			ItemJson->SetStringField(TEXT("ItemTransform"), SaveItemData.ItemTransform.ToString());
			ItemJson->SetBoolField(TEXT("bIsHeld"), SaveItemData.bIsHeld);
			ItemJson->SetBoolField(TEXT("bIsAttached"), SaveItemData.bIsAttached);
			ItemJson->SetBoolField(TEXT("bHasSimulatedPhysics"), SaveItemData.bHasSimulatedPhysics);
			ItemJson->SetBoolField(TEXT("bIsBroken"), SaveItemData.bIsBroken);

			ItemsJsonArray.Add(MakeShared<FJsonValueObject>(ItemJson));
		}

		TSharedPtr<FJsonObject> SaveChoreJsonObject = MakeShared<FJsonObject>();
		SaveChoreJsonObject->SetArrayField(TEXT("Items"), ItemsJsonArray);
		SaveChoreJsonObject->SetNumberField(TEXT("Status"), static_cast<int32>(SaveChoreData.ChoreStatus));

		Root->SetObjectField(ZoneKey, SaveChoreJsonObject);
	}

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	OutData.SerializedData = JsonString;
}

void UGIS_ChorePlacement::ApplySaveData(const FSubsystemSaveData& InData)
{

	bIsLoadComplete = false;

	CachedChorePlacementZoneData.Empty();

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

	for (const auto& ZonePair : Root->Values)
	{
		FSaveChoreData SaveChoreData;

		const TSharedPtr<FJsonObject>* ChoreJsonObject = nullptr;
		if (!ZonePair.Value->TryGetObject(ChoreJsonObject))
		{
			continue;
		}

		const TArray<TSharedPtr<FJsonValue>>* ItemsJsonArray = nullptr;
		if (!(*ChoreJsonObject)->TryGetArrayField(TEXT("Items"), ItemsJsonArray))
		{
			continue;
		}

		int32 StatusInt = 0;
		if (!(*ChoreJsonObject)->TryGetNumberField(TEXT("Status"), StatusInt))
		{
			continue;
		}
		SaveChoreData.ChoreStatus = static_cast<EChoreStatus>(StatusInt);

		for (const TSharedPtr<FJsonValue>& ItemJsonValue : *ItemsJsonArray)
		{
			const TSharedPtr<FJsonObject>* ItemJsonObjPtr = nullptr;

			if (!ItemJsonValue->TryGetObject(ItemJsonObjPtr))
			{
				continue;
			}
			TSharedPtr<FJsonObject> ItemJsonObj = *ItemJsonObjPtr;

			FSaveItemData SaveItemData;

			FString ItemIdStr;
			ItemJsonObj->TryGetStringField(TEXT("ItemId"), ItemIdStr);
			SaveItemData.ItemId = ItemIdStr.IsEmpty() ? FGuid() : FGuid(ItemIdStr);

			FString AttachingDropZoneIdStr;
			ItemJsonObj->TryGetStringField(TEXT("AttachingDropZoneId"), AttachingDropZoneIdStr);
			SaveItemData.AttachingDropZoneId = AttachingDropZoneIdStr.IsEmpty() ? FGuid() : FGuid(AttachingDropZoneIdStr);

			if (!SaveItemData.AttachingDropZoneId.IsValid())
			{
				FString TransformString;
				ItemJsonObj->TryGetStringField(TEXT("ItemTransform"), TransformString);
				if (!TransformString.IsEmpty())
				{
					if (!SaveItemData.ItemTransform.InitFromString(TransformString))
					{
						UE_LOG(LogTemp, Log, TEXT("Failed to parse ItemTransform from string: %s"), *TransformString);
					}
				}
				bool TempBool = false;
				ItemJsonObj->TryGetBoolField(TEXT("bIsHeld"), TempBool);
				SaveItemData.bIsHeld = TempBool;
				TempBool = false;
				ItemJsonObj->TryGetBoolField(TEXT("bIsAttached"), TempBool);
				SaveItemData.bIsAttached = TempBool;
				TempBool = false;
				ItemJsonObj->TryGetBoolField(TEXT("bHasSimulatedPhysics"), TempBool);
				SaveItemData.bHasSimulatedPhysics = TempBool;
				TempBool = false;
				ItemJsonObj->TryGetBoolField(TEXT("bIsBroken"), TempBool);
				SaveItemData.bIsBroken = TempBool;
			}

			SaveChoreData.SaveItemData.Add(SaveItemData);
		}

		CachedChorePlacementZoneData.Add(FGuid(ZonePair.Key), SaveChoreData);
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

void UGIS_ChorePlacement::ApplyChorePlacementData_Implementation(AActor* ChorePlacementZone)
{
	if (!ChorePlacementZone || !ChorePlacementZone->Implements<UI_SaveableObject>())
	{
		return;
	}

	FGuid ActorID;
	if (UFloorAssignmentComponent* FloorAssignmentComp = ChorePlacementZone->FindComponentByClass<UFloorAssignmentComponent>())
	{
		ActorID = FloorAssignmentComp->GetItemId();
	}

	FSaveChoreData* FoundData = CachedChorePlacementZoneData.Find(ActorID);

	if (!FoundData)
	{
		return;
	}

	II_SaveableObject::Execute_ApplyChorePlacementSaveData(ChorePlacementZone, *FoundData);
}
