#include "RealEstate/A_RealEstatePaperWork.h"
#include "FloorAssignmentComponent.h"
#include "InteractiveItemComponent.h"
#include "InteractivePickerComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "RealEstate/W_RealEstateQuestionnaire.h"
#include "RealEstate/DA_RealEstateQuestionnaire.h"
#include "SaveSubsystems/GIS_RealEstatePaperWork.h"
#include "Kismet/GameplayStatics.h"

AA_RealEstatePaperWork::AA_RealEstatePaperWork()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	FloorAssignmentComponent = CreateDefaultSubobject<UFloorAssignmentComponent>(TEXT("FloorAssignmentComponent"));
	InteractiveItemComponent = CreateDefaultSubobject<UInteractiveItemComponent>(TEXT("InteractiveItemComponent"));

	MeshComponent->SetupAttachment(RootComponent);
}

void AA_RealEstatePaperWork::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (IsValid(QuestionnaireDataAsset))
	{
		QuestionsDataMap = QuestionnaireDataAsset->Questions;
	}
}

void AA_RealEstatePaperWork::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_RealEstatePaperWork::BeginPlay()
{
	Super::BeginPlay();

	InteractiveItemComponent->OnInteractionPressKeyEvent.AddDynamic(this, &AA_RealEstatePaperWork::OnInteract);
	InteractiveItemComponent->OnInteractiveReceiveFocusEvent.AddDynamic(this, &AA_RealEstatePaperWork::OnInteractiveReceiveFocus);
}

void AA_RealEstatePaperWork::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

}

bool AA_RealEstatePaperWork::CheckIfCompleted()
{
	int32 RightAnswersCounter = 0;

	for (const auto& QuestionData : QuestionsDataMap)
	{
		RightAnswersCounter += QuestionData.Value.bIsPassed;
	}

	return RightAnswersCounter >= FMath::CeilToInt(QuestionsDataMap.Num() * RightAnswersPercentage / 100.0f);
}

void AA_RealEstatePaperWork::OnInteract(UInteractivePickerComponent* Picker)
{
	if (GetChoreStatus() == EChoreStatus::Inactive || GetChoreStatus() == EChoreStatus::Completed)
	{
		return;
	}

	if (QuestionnaireWidgetClass)
	{
		if (!IsValid(QuestionnaireWidget))
		{
			QuestionnaireWidget = CreateWidget<UW_RealEstateQuestionnaire>(GetWorld(), QuestionnaireWidgetClass);

			if (!IsValid(QuestionnaireWidget))
			{
				return;
			}

			QuestionnaireWidget->OnAnswerSelected.AddDynamic(this, &AA_RealEstatePaperWork::HandleAnswerSelected);
		}
		if (IsValid(QuestionnaireWidget))
		{
			QuestionnaireWidget->UpdateQuestionsData(ChoreId, QuestionsDataMap);
			QuestionnaireWidget->AddToViewport(9999);

			APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);

			if (IsValid(PC))
			{
				PC->SetShowMouseCursor(true);

				FInputModeUIOnly InputMode;
				InputMode.SetWidgetToFocus(QuestionnaireWidget->TakeWidget());
				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				PC->SetInputMode(InputMode);
			}
		}
	}
}

void AA_RealEstatePaperWork::OnInteractiveReceiveFocus(AActor* WhoInteract)
{
	if (GetChoreStatus() == EChoreStatus::Inactive)
	{
		InteractiveItemComponent->SetTooltip(FText::FromString("Questionnaire is not active"));
	}
	else if (GetChoreStatus() == EChoreStatus::Completed)
	{
		InteractiveItemComponent->SetTooltip(FText::FromString("Questionnaire is completed"));
	}
	else
	{
		InteractiveItemComponent->SetTooltip(FText::FromString("Press F to fill the questionnaire"));
	}
}

void AA_RealEstatePaperWork::UpdateWidgetData()
{
	if (IsValid(QuestionnaireWidget))
	{
		QuestionnaireWidget->UpdateQuestionsData(ChoreId, QuestionsDataMap);
	}
}

void AA_RealEstatePaperWork::SetChoreStatus(EChoreStatus NewStatus)
{
	if (ChoreStatus == NewStatus)
	{
		return;
	}

	ChoreStatus = NewStatus;

	switch (ChoreStatus)
	{
	case EChoreStatus::Inactive:
	{
		UE_LOG(LogTemp, Log, TEXT("RealEstateQuestionnaire %s is now Inactive"), *ChoreId.ToString());
		break;
	}
	case EChoreStatus::Active:
	{
		UE_LOG(LogTemp, Log, TEXT("RealEstateQuestionnaire %s is now Active"), *ChoreId.ToString());
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("RealEstateQuestionnaire %s is now Active"), *ChoreId.ToString()));
		break;
	}
	case EChoreStatus::Completed:
	{
		UE_LOG(LogTemp, Log, TEXT("RealEstateQuestionnaire %s is now Completed"), *ChoreId.ToString());
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("RealEstateQuestionnaire %s is now Completed"), *ChoreId.ToString()));
		if (IsValid(QuestionnaireWidget))
		{
			QuestionnaireWidget->RemoveFromParent();
			QuestionnaireWidget = nullptr;
		}
		break;
	}
	case EChoreStatus::Failed:
	{
		UE_LOG(LogTemp, Log, TEXT("RealEstateQuestionnaire %s is now Failed"), *ChoreId.ToString());
		break;
	}
	case EChoreStatus::Abandoned:
	{
		UE_LOG(LogTemp, Log, TEXT("RealEstateQuestionnaire %s is now Abandoned"), *ChoreId.ToString());
		break;
	}
	case EChoreStatus::Unavailable:
	{
		UE_LOG(LogTemp, Log, TEXT("RealEstateQuestionnaire %s is now Unavailable"), *ChoreId.ToString());
		break;
	}
	case EChoreStatus::Available:
	{
		UE_LOG(LogTemp, Log, TEXT("RealEstateQuestionnaire %s is now Available"), *ChoreId.ToString());
		break;
	}
	case EChoreStatus::Offered:
	{
		UE_LOG(LogTemp, Log, TEXT("RealEstateQuestionnaire %s is now Offered"), *ChoreId.ToString());
		break;
	}
	case EChoreStatus::Accepted:
	{
		UE_LOG(LogTemp, Log, TEXT("RealEstateQuestionnaire %s is now Accepted"), *ChoreId.ToString());
		break;
	}
	case EChoreStatus::WaitingToStart:
	{
		UE_LOG(LogTemp, Log, TEXT("RealEstateQuestionnaire %s is now WaitingToStart"), *ChoreId.ToString());
		break;
	}
	case EChoreStatus::Expired:
	{
		UE_LOG(LogTemp, Log, TEXT("RealEstateQuestionnaire %s is now Expired"), *ChoreId.ToString());
		break;
	}
	case EChoreStatus::RetryAvailable:
	{
		UE_LOG(LogTemp, Log, TEXT("RealEstateQuestionnaire %s is now RetryAvailable"), *ChoreId.ToString());
		break;
	}
	case EChoreStatus::PendingReaccept:
	{
		UE_LOG(LogTemp, Log, TEXT("RealEstateQuestionnaire %s is now PendingReaccept"), *ChoreId.ToString());
		break;
	}
	case EChoreStatus::Started:
	{
		OnStarted();
		UE_LOG(LogTemp, Log, TEXT("RealEstateQuestionnaire %s is now Started"), *ChoreId.ToString());
		break;
	}
	default:
	{
		UE_LOG(LogTemp, Warning, TEXT("RealEstateQuestionnaire %s has an unknown status"), *ChoreId.ToString());
		break;
	}
	}
}

EChoreStatus AA_RealEstatePaperWork::GetChoreStatus()const
{
	return ChoreStatus;
}

void AA_RealEstatePaperWork::HandleAnswerSelected(FString Question, int32 AnswerIndex, bool IsSelected)
{
	SetChoreStatus(EChoreStatus::Started);

	FQuestionDataStruct* QuestionData = QuestionsDataMap.Find(Question);
	if (QuestionData && QuestionData->AnswersArray.IsValidIndex(AnswerIndex))
	{
		QuestionData->AnswersArray[AnswerIndex].bIsChecked = IsSelected;
		QuestionData->bIsPassed = false;

		if (IsSelected && QuestionData->AnswersArray[AnswerIndex].bIsSuitable)
		{
			QuestionData->bIsPassed = true;
		}
	}

	if (CheckIfCompleted() && !bIsDeferredEvaluate)
	{
		SetChoreStatus(EChoreStatus::Completed);
	}
}

TMap<FString, FQuestionDataStruct> AA_RealEstatePaperWork::CollectRealEstatePaperWorkSaveData_Implementation()
{
	// to do status save
	return QuestionsDataMap;
}

void AA_RealEstatePaperWork::ApplyRealEstatePaperWorkSaveData_Implementation(UGIS_RealEstatePaperWork* GIS)
{
	// to do status save

	QuestionsDataMap = GIS->CachedRealEstateData;

	UpdateWidgetData();
}

FString AA_RealEstatePaperWork::CollectIntermediateSaveData()
{
	FSubsystemSaveData SaveData;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGIS_RealEstatePaperWork* RealEstateSys = GI->GetSubsystem<UGIS_RealEstatePaperWork>())
		{
			RealEstateSys->CollectSaveData(SaveData);
		}
	}
	return SaveData.SerializedData;
}

void AA_RealEstatePaperWork::ApplyIntermediateSaveData(FString Jsonstring)
{
	FSubsystemSaveData SaveData;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGIS_RealEstatePaperWork* RealEstateSys = GI->GetSubsystem<UGIS_RealEstatePaperWork>())
		{
			SaveData.SerializedData = Jsonstring;
			RealEstateSys->ApplySaveData(SaveData);
			ApplyRealEstatePaperWorkSaveData_Implementation(RealEstateSys);
		}
	}
}
