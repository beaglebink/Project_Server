#include "SaveSubsystems/GIS_Container.h"
#include <GameSaveSubsystem.h>
#include "Inventory/AC_Container.h"
#include "FloorAssignmentComponent.h"

void UGIS_Container::Initialize(FSubsystemCollectionBase& Collection)
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

void UGIS_Container::Deinitialize()
{
	// Отписка от сохранения
	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->UnregisterSaveableSubsystem(this);
	}
	Super::Deinitialize();
}

void UGIS_Container::CollectSaveData(FSubsystemSaveData& OutData)
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

void UGIS_Container::ApplySaveData(const FSubsystemSaveData& InData)
{

	bIsLoadComplete = false;

	CachedContainerData.Empty();

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

void UGIS_Container::ClearTransientData_Implementation()
{
	SaveableObjects.Empty();
	UE_LOG(LogTemp, Log, TEXT("ContainerSubsystem transient data cleared"));
}

void UGIS_Container::AddSaveableObject(UObject* NewSaveableObject)
{
	if (NewSaveableObject && NewSaveableObject->Implements<UI_SaveableObject>())
	{
		SaveableObjects.AddUnique(NewSaveableObject);
	}
}

void UGIS_Container::ApplyContainerData_Implementation(UActorComponent* ContainerComponent)
{
	if (!ContainerComponent || !ContainerComponent->GetOwner() || !ContainerComponent->Implements<UI_SaveableObject>())
	{
		return;
	}

	FGuid ActorID;
	if (UFloorAssignmentComponent* FloorAssignmentComp = ContainerComponent->GetOwner()->FindComponentByClass<UFloorAssignmentComponent>())
	{
		ActorID = FloorAssignmentComp->GetItemId();
	}

	int32* FoundData = CachedContainerData.Find(ActorID);

	if (!FoundData)
	{
		return;
	}

	II_SaveableObject::Execute_ApplyContainerSaveData(ContainerComponent, *FoundData);
}
