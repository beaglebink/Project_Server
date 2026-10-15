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

		FSaveItemData SaveDataMap = II_SaveableObject::Execute_CollectChorePlacementSaveData(SaveableObject);



	}

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	OutData.SerializedData = JsonString;
}

void UGIS_ChorePlacement::ApplySaveData(const FSubsystemSaveData& InData)
{

	bIsLoadComplete = false;

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

	const FSaveItemData* FoundData = CachedChorePlacementZoneData.Find(ActorID);

	if (!FoundData)
	{
		return;
	}

	II_SaveableObject::Execute_ApplyChorePlacementSaveData(ChorePlacementZone, *FoundData);
}
