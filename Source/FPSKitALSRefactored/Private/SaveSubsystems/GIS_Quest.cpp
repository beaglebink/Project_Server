#include "SaveSubsystems/GIS_Quest.h"
#include <GameSaveSubsystem.h>
#include "Quest/AC_QuestHandleComponent.h"
#include "FloorAssignmentComponent.h"

void UGIS_Quest::Initialize(FSubsystemCollectionBase& Collection)
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

void UGIS_Quest::Deinitialize()
{
	// Отписка от сохранения
	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->UnregisterSaveableSubsystem(this);
	}
	Super::Deinitialize();
}

void UGIS_Quest::CollectSaveData(FSubsystemSaveData& OutData)
{
	OutData.SubsystemName = GetSaveSubsystemName();

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	for (UObject* SaveableObject : SaveableObjects)
	{
		if (!SaveableObject || !SaveableObject->Implements<UI_SaveableObject>())
		{
			continue;
		}


	}

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	OutData.SerializedData = JsonString;
}

void UGIS_Quest::ApplySaveData(const FSubsystemSaveData& InData)
{

	bIsLoadComplete = false;

	CachedQuestData.Empty();

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

	}

	bIsLoadComplete = true;
}

void UGIS_Quest::ClearTransientData_Implementation()
{
	SaveableObjects.Empty();
	UE_LOG(LogTemp, Log, TEXT("QuestSubsystem transient data cleared"));
}

void UGIS_Quest::AddSaveableObject(UObject* NewSaveableObject)
{
	if (NewSaveableObject && NewSaveableObject->Implements<UI_SaveableObject>())
	{
		SaveableObjects.AddUnique(NewSaveableObject);
	}
}

void UGIS_Quest::ApplyQuestData_Implementation(UActorComponent* QuestComponent)
{
	if (!QuestComponent || !QuestComponent->GetOwner() || !QuestComponent->Implements<UI_SaveableObject>())
	{
		return;
	}

	FGuid ActorID;
	if (UFloorAssignmentComponent* FloorAssignmentComp = QuestComponent->GetOwner()->FindComponentByClass<UFloorAssignmentComponent>())
	{
		ActorID = FloorAssignmentComp->GetItemId();
	}

	int32* FoundData = CachedQuestData.Find(ActorID);

	if (!FoundData)
	{
		return;
	}

	II_SaveableObject::Execute_ApplyQuestSaveData(QuestComponent, *FoundData);
}