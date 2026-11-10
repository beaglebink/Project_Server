#include "SaveSubsystems/GIS_RealEstatePaperWork.h"
#include <GameSaveSubsystem.h>
#include "RealEstate/A_RealEstatePaperWork.h"

void UGIS_RealEstatePaperWork::Initialize(FSubsystemCollectionBase& Collection)
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

void UGIS_RealEstatePaperWork::Deinitialize()
{
	// Отписка от сохранения
	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->UnregisterSaveableSubsystem(this);
	}
	Super::Deinitialize();
}

void UGIS_RealEstatePaperWork::CollectSaveData(FSubsystemSaveData& OutData)
{
	OutData.SubsystemName = GetSaveSubsystemName();

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	for (UObject* SaveableObject : SaveableObjects)
	{
		if (!SaveableObject || !SaveableObject->Implements<UI_SaveableObject>())
		{
			continue;
		}

		TMap<FName, int32> QuestSaveData = II_SaveableObject::Execute_CollectRealEstatePaperWorkSaveData(SaveableObject);;

		for (const auto& [QuestName, QuestData] : QuestSaveData)
		{
			//TSharedPtr<FJsonObject> QuestSaveJsonObject = MakeShared<FJsonObject>();

			//QuestSaveJsonObject->SetNumberField(TEXT("Status"), static_cast<int32>(QuestData.QuestStatus));

			//TArray<TSharedPtr<FJsonValue>> QuestStagesJsonArray;
			//for (const auto& QuestStages : QuestData.QuestsArray)
			//{
			//	TSharedPtr<FJsonObject> QuestStageSaveJsonObject = MakeShared<FJsonObject>();

			//	QuestStageSaveJsonObject->SetBoolField(TEXT("IsCompleted"), QuestStages.bIsCompleted);

			//	TArray<TSharedPtr<FJsonValue>> QuestStepsJsonArray;
			//	for (const auto& QuestStep : QuestStages.QuestStepDataArray)
			//	{
			//		TSharedPtr<FJsonObject> QuestStepSaveJsonObject = MakeShared<FJsonObject>();

			//		QuestStepSaveJsonObject->SetStringField(TEXT("StepTag"), QuestStep.QuestStepTag.ToString());
			//		QuestStepSaveJsonObject->SetNumberField(TEXT("StepValue"), QuestStep.QuestStepValue);
			//		QuestStepSaveJsonObject->SetBoolField(TEXT("IsDynamic"), QuestStep.bIsDynamic);
			//		QuestStepSaveJsonObject->SetBoolField(TEXT("ShouldBeRemovedFromInventoryOnComplete"), QuestStep.bShouldBeRemovedFromInventoryOnComplete);
			//		QuestStepSaveJsonObject->SetBoolField(TEXT("IsChecked"), QuestStep.bIsChecked);

			//		QuestStepsJsonArray.Add(MakeShared<FJsonValueObject>(QuestStepSaveJsonObject));
			//	}

			//	QuestStageSaveJsonObject->SetArrayField(TEXT("Steps"), QuestStepsJsonArray);

			//	QuestStagesJsonArray.Add(MakeShared<FJsonValueObject>(QuestStageSaveJsonObject));
			//}

			//QuestSaveJsonObject->SetArrayField(TEXT("Stages"), QuestStagesJsonArray);

			//Root->SetObjectField(QuestName.ToString(), QuestSaveJsonObject);
		}
	}

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	OutData.SerializedData = JsonString;
}

void UGIS_RealEstatePaperWork::ApplySaveData(const FSubsystemSaveData& InData)
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

	for (const auto& [QuestName, QuestSaveData] : Root->Values)
	{
		//FQuestStepDataStruct QuestStepDataStruct;

		//const TSharedPtr<FJsonObject>* QuestSaveJsonObject = nullptr;
		//if (!QuestSaveData->TryGetObject(QuestSaveJsonObject))
		//{
		//	continue;
		//}
		//int32 QuestStatusNumbered;
		//if (!(*QuestSaveJsonObject)->TryGetNumberField(TEXT("Status"), QuestStatusNumbered))
		//{
		//	continue;
		//}
		//QuestStepDataStruct.QuestStatus = static_cast<EChoreStatus>(QuestStatusNumbered);

		//const TArray<TSharedPtr<FJsonValue>>* QuestStagesJsonArray = nullptr;
		//if (!(*QuestSaveJsonObject)->TryGetArrayField(TEXT("Stages"), QuestStagesJsonArray))
		//{
		//	continue;
		//}
		//for (const auto& QuestStageJsonValue : *QuestStagesJsonArray)
		//{
		//	FQuestStepDataArray QuestStage;

		//	const TSharedPtr<FJsonObject>* QuestStageJsonObject = nullptr;
		//	if (!QuestStageJsonValue->TryGetObject(QuestStageJsonObject))
		//	{
		//		continue;
		//	}

		//	bool bIsCompleted = false;
		//	if (!(*QuestStageJsonObject)->TryGetBoolField(TEXT("IsCompleted"), bIsCompleted))
		//	{
		//		continue;
		//	}
		//	QuestStage.bIsCompleted = bIsCompleted;

		//	const TArray<TSharedPtr<FJsonValue>>* QuestStepsJsonArray = nullptr;
		//	if (!(*QuestStageJsonObject)->TryGetArrayField(TEXT("Steps"), QuestStepsJsonArray))
		//	{
		//		continue;
		//	}
		//	for (const auto& QuestStepJsonValue : *QuestStepsJsonArray)
		//	{
		//		FQuestStepData QuestStep;

		//		const TSharedPtr<FJsonObject>* QuestStepJsonObject = nullptr;
		//		if (!QuestStepJsonValue->TryGetObject(QuestStepJsonObject))
		//		{
		//			continue;
		//		}

		//		FString TagString;
		//		if (!(*QuestStepJsonObject)->TryGetStringField(TEXT("StepTag"), TagString))
		//		{
		//			continue;
		//		}
		//		QuestStep.QuestStepTag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);

		//		if (!(*QuestStepJsonObject)->TryGetNumberField(TEXT("StepValue"), QuestStep.QuestStepValue))
		//		{
		//			continue;
		//		}

		//		bool bIsDynamic = false;
		//		if (!(*QuestStepJsonObject)->TryGetBoolField(TEXT("IsDynamic"), bIsDynamic))
		//		{
		//			continue;
		//		}
		//		QuestStep.bIsDynamic = bIsDynamic;

		//		bool bShouldBeRemovedFromInventoryOnComplete = false;
		//		if (!(*QuestStepJsonObject)->TryGetBoolField(TEXT("ShouldBeRemovedFromInventoryOnComplete"), bShouldBeRemovedFromInventoryOnComplete))
		//		{
		//			continue;
		//		}
		//		QuestStep.bShouldBeRemovedFromInventoryOnComplete = bShouldBeRemovedFromInventoryOnComplete;

		//		bool bIsChecked = false;
		//		if (!(*QuestStepJsonObject)->TryGetBoolField(TEXT("IsChecked"), bIsChecked))
		//		{
		//			continue;
		//		}
		//		QuestStep.bIsChecked = bIsChecked;

		//		QuestStage.QuestStepDataArray.Add(QuestStep);
		//	}

		//	QuestStepDataStruct.QuestsArray.Add(QuestStage);
		//}

		//CachedQuestData.FindOrAdd(FName(*QuestName), QuestStepDataStruct);
	}

	bIsLoadComplete = true;
}

void UGIS_RealEstatePaperWork::ClearTransientData_Implementation()
{
	SaveableObjects.Empty();
	UE_LOG(LogTemp, Log, TEXT("RealEstatePaperWork transient data cleared"));
}

void UGIS_RealEstatePaperWork::AddSaveableObject(UObject* NewSaveableObject)
{
	if (NewSaveableObject && NewSaveableObject->Implements<UI_SaveableObject>())
	{
		SaveableObjects.AddUnique(NewSaveableObject);
	}
}