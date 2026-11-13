#include "SaveSubsystems/GIS_FlyerPosting.h"
#include <GameSaveSubsystem.h>
#include "FlyerPosting/A_FlyerPostingBase.h"

void UGIS_FlyerPosting::Initialize(FSubsystemCollectionBase& Collection)
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

void UGIS_FlyerPosting::Deinitialize()
{
	// Отписка от сохранения
	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->UnregisterSaveableSubsystem(this);
	}
	Super::Deinitialize();
}

void UGIS_FlyerPosting::CollectSaveData(FSubsystemSaveData& OutData)
{
	OutData.SubsystemName = GetSaveSubsystemName();

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	for (UObject* SaveableObject : SaveableObjects)
	{
		if (!SaveableObject || !SaveableObject->Implements<UI_SaveableObject>())
		{
			continue;
		}

		//FQuestionSaveDataStruct FlyerPostingSaveData = II_SaveableObject::Execute_CollectFlyerPostingSaveData(SaveableObject);;

		//TSharedPtr<FJsonObject> FlyerPostingSaveJsonObject = MakeShared<FJsonObject>();

		//FlyerPostingSaveJsonObject->SetNumberField(TEXT("ChoreStatus"), static_cast<int32>(FlyerPostingSaveData.ChoreStatus));

		//for (const auto& [Question, QuestionData] : FlyerPostingSaveData.Questions)
		//{
		//	TSharedPtr<FJsonObject> QuestionSaveJsonObject = MakeShared<FJsonObject>();

		//	TArray<TSharedPtr<FJsonValue>> AnswersJsonArray;

		//	for (const auto& AnswerData : QuestionData.AnswersArray)
		//	{
		//		TSharedPtr<FJsonObject> AnswerSaveJsonObject = MakeShared<FJsonObject>();

		//		FString SerializedAnswerText;
		//		FTextStringHelper::WriteToBuffer(SerializedAnswerText, AnswerData.AnswerText, false, false);
		//		AnswerSaveJsonObject->SetStringField(TEXT("AnswerText"), SerializedAnswerText);

		//		AnswerSaveJsonObject->SetBoolField(TEXT("bIsSuitable"), AnswerData.bIsSuitable);
		//		AnswerSaveJsonObject->SetBoolField(TEXT("bIsChecked"), AnswerData.bIsChecked);

		//		AnswersJsonArray.Add(MakeShared<FJsonValueObject>(AnswerSaveJsonObject));
		//	}
		//	QuestionSaveJsonObject->SetArrayField(TEXT("AnswersArray"), AnswersJsonArray);
		//	QuestionSaveJsonObject->SetBoolField(TEXT("bIsPassed"), QuestionData.bIsPassed);

		//	FlyerPostingSaveJsonObject->SetObjectField(Question, QuestionSaveJsonObject);
		//}

		//FString FlyerPostingObjectName = II_SaveableObject::Execute_GetFloorAssignmentComponentIdAsString(SaveableObject);
		//Root->SetObjectField(FlyerPostingObjectName, FlyerPostingSaveJsonObject);
	}

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	OutData.SerializedData = JsonString;
}

void UGIS_FlyerPosting::ApplySaveData(const FSubsystemSaveData& InData)
{

	bIsLoadComplete = false;

	CachedFlyerPostingData.Empty();

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

	for (const auto& [RealEstateObjectId, RealEstateSaveJsonData] : Root->Values)
	{
		//FQuestionSaveDataStruct QuestionSaveDataStruct;

		//int32 RealEstateStatusNumbered;
		//if (!RealEstateSaveJsonData->AsObject()->TryGetNumberField(TEXT("ChoreStatus"), RealEstateStatusNumbered))
		//{
		//	continue;
		//}
		//QuestionSaveDataStruct.ChoreStatus = static_cast<EChoreStatus>(RealEstateStatusNumbered);

		//for (const auto& [Question, QuestionJsonData] : RealEstateSaveJsonData->AsObject()->Values)
		//{
		//	FQuestionDataStruct QuestionDataStruct;

		//	if (!QuestionJsonData.IsValid() || QuestionJsonData->Type != EJson::Object)
		//	{
		//		continue;
		//	}
		//	const TSharedPtr<FJsonObject> QuestionSaveJsonObject = QuestionJsonData->AsObject();
		//	if (!QuestionSaveJsonObject.IsValid())
		//	{
		//		continue;
		//	}

		//	const TArray<TSharedPtr<FJsonValue>>* AnswersJsonArray = nullptr;
		//	if (!QuestionSaveJsonObject->TryGetArrayField(TEXT("AnswersArray"), AnswersJsonArray))
		//	{
		//		continue;
		//	}
		//	for (const auto& AnswerJsonValue : *AnswersJsonArray)
		//	{
		//		FAnswerDataStruct AnswerDataStruct;

		//		if (!AnswerJsonValue.IsValid() || AnswerJsonValue->Type != EJson::Object)
		//		{
		//			continue;
		//		}
		//		TSharedPtr<FJsonObject> AnswerJsonObject = AnswerJsonValue->AsObject();

		//		FString AnswerText;
		//		if (!AnswerJsonObject->TryGetStringField(TEXT("AnswerText"), AnswerText))
		//		{
		//			continue;
		//		}
		//		bool bIsSuitable = false;
		//		if (!AnswerJsonObject->TryGetBoolField(TEXT("bIsSuitable"), bIsSuitable))
		//		{
		//			continue;
		//		}
		//		bool bIsChecked = false;
		//		if (!AnswerJsonObject->TryGetBoolField(TEXT("bIsChecked"), bIsChecked))
		//		{
		//			continue;
		//		}

		//		if (!FTextStringHelper::ReadFromBuffer(*AnswerText, AnswerDataStruct.AnswerText, TEXT(""), TEXT(""), false))
		//		{
		//			continue;
		//		}

		//		AnswerDataStruct.bIsSuitable = bIsSuitable;
		//		AnswerDataStruct.bIsChecked = bIsChecked;
		//		QuestionDataStruct.AnswersArray.Add(AnswerDataStruct);
		//	}

		//	bool bIsPassed = false;
		//	if (!QuestionSaveJsonObject->TryGetBoolField(TEXT("bIsPassed"), bIsPassed))
		//	{
		//		continue;
		//	}
		//	QuestionDataStruct.bIsPassed = bIsPassed;

		//	QuestionSaveDataStruct.Questions.Add(Question, QuestionDataStruct);
		//}

		//CachedFlyerPostingData.Add(FlyerPostingObjectId, QuestionSaveDataStruct);
	}

	bIsLoadComplete = true;
}

void UGIS_FlyerPosting::ClearTransientData_Implementation()
{
	SaveableObjects.Empty();
	UE_LOG(LogTemp, Log, TEXT("FlyerPosting transient data cleared"));
}

void UGIS_FlyerPosting::AddSaveableObject(UObject* NewSaveableObject)
{
	if (NewSaveableObject && NewSaveableObject->Implements<UI_SaveableObject>())
	{
		SaveableObjects.AddUnique(NewSaveableObject);
	}
}