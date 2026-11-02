#include "Quest/AC_QuestHandleComponent.h"
#include "Engine/AssetManager.h"

UAC_QuestHandleComponent::UAC_QuestHandleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UAC_QuestHandleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{

}

void UAC_QuestHandleComponent::BeginPlay()
{

}

void UAC_QuestHandleComponent::EndPlay(const EEndPlayReason::Type Reason)
{

}

void UAC_QuestHandleComponent::LoadQuestData(FName QuestName)
{
	const FPrimaryAssetId AssetId(FPrimaryAssetType(TEXT("PDA_Quest")), QuestName);

	UAssetManager& AssetManager = UAssetManager::Get();

	AssetManager.LoadPrimaryAsset(AssetId, TArray<FName>(), FStreamableDelegate::CreateWeakLambda(this, [this, AssetId]()
		{
			UPDA_Quest* QuestAsset = Cast<UPDA_Quest>(UAssetManager::Get().GetPrimaryAssetObject(AssetId));

			if (!QuestAsset)
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to load quest asset: %s"), *AssetId.ToString());
				return;
			}

			FQuestStepDataStruct RuntimeQuestData;
			RuntimeQuestData.QuestsArray = QuestAsset->QuestsArray;
			QuestDataMap.Add(AssetId.PrimaryAssetName, MoveTemp(RuntimeQuestData));
		})
	);
}

void UAC_QuestHandleComponent::SetChoreStatus(FName QuestName, EChoreStatus NewStatus)
{
	if (QuestDataMap.Contains(QuestName))
	{
		if (QuestDataMap[QuestName].QuestStatus != NewStatus)
		{
			QuestDataMap[QuestName].QuestStatus = NewStatus;

			switch (QuestDataMap[QuestName].QuestStatus)
			{
			case EChoreStatus::Inactive:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Inactive"), *QuestName.ToString());
				break;
			}
			case EChoreStatus::Active:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Active"), *QuestName.ToString());
				break;
			}
			case EChoreStatus::Completed:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Completed"), *QuestName.ToString());
				break;
			}
			case EChoreStatus::Failed:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Failed"), *QuestName.ToString());
				break;
			}
			case EChoreStatus::Abandoned:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Abandoned"), *QuestName.ToString());
				break;
			}
			case EChoreStatus::Unavailable:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Unavailable"), *QuestName.ToString());
				break;
			}
			case EChoreStatus::Available:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Available"), *QuestName.ToString());
				break;
			}
			case EChoreStatus::Offered:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Offered"), *QuestName.ToString());
				break;
			}
			case EChoreStatus::Accepted:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Accepted"), *QuestName.ToString());
				break;
			}
			case EChoreStatus::WaitingToStart:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now WaitingToStart"), *QuestName.ToString());
				break;
			}
			case EChoreStatus::Expired:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Expired"), *QuestName.ToString());
				break;
			}
			case EChoreStatus::RetryAvailable:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now RetryAvailable"), *QuestName.ToString());
				break;
			}
			case EChoreStatus::PendingReaccept:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now PendingReaccept"), *QuestName.ToString());
				break;
			}
			case EChoreStatus::Started:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Started"), *QuestName.ToString());
				break;
			}
			default:
			{
				UE_LOG(LogTemp, Warning, TEXT("Quest %s has an unknown status"), *QuestName.ToString());
				break;
			}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Quest not found: %s"), *QuestName.ToString());
	}
}

EChoreStatus UAC_QuestHandleComponent::GetChoreStatus(FName ChoreID)const
{
	if (QuestDataMap.Contains(ChoreID))
	{
		return QuestDataMap[ChoreID].QuestStatus;
	}

	return EChoreStatus::Inactive;
}