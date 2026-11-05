#include "Quest/AC_QuestHandleComponent.h"
#include "Engine/AssetManager.h"
#include "CoreGameplay/LocationTrackerSubsystem/LocationTrackerSubsystem.h"
#include "Inventory/AC_Inventory.h"

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

bool UAC_QuestHandleComponent::LoadQuestData(FName QuestName)
{
	const FPrimaryAssetId AssetId(FPrimaryAssetType(TEXT("PDA_Quest")), QuestName);
	UAssetManager& AssetManager = UAssetManager::Get();

	TArray<FPrimaryAssetId> RegisteredAssets;

	AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType(TEXT("PDA_Quest")), RegisteredAssets);

	const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetId);

	if (!AssetPath.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Quest asset not registered: %s"), *AssetId.ToString());
		return false;
	}

	TSharedPtr<FStreamableHandle> Handle = AssetManager.GetStreamableManager().RequestSyncLoad(AssetPath);

	if (!Handle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load quest asset: %s"), *AssetId.ToString());
		return false;
	}

	UPDA_Quest* QuestAsset = Cast<UPDA_Quest>(Handle->GetLoadedAsset());

	if (!QuestAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid quest asset: %s"), *AssetId.ToString());
		return false;
	}

	FQuestStepDataStruct RuntimeQuestData;
	RuntimeQuestData.QuestsArray = QuestAsset->QuestsArray;
	QuestDataMap.Add(AssetId.PrimaryAssetName, MoveTemp(RuntimeQuestData));

	return true;
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

FString UAC_QuestHandleComponent::CollectIntermediateSaveData(FName ChoreID)
{
	//FSubsystemSaveData SaveData;
	//if (UGameInstance* GI = GetGameInstance())
	//{
	//	if (UGIS_Quest* QuestSys = GI->GetSubsystem<UGIS_Quest>())
	//	{
	//		QuestSys->CollectSaveData(SaveData);
	//	}
	//}
	//return SaveData.SerializedData;
	return FString();
}

void UAC_QuestHandleComponent::ApplyIntermediateSaveData(FName ChoreID, FString Jsonstring)
{
	//FSubsystemSaveData SaveData;
	//if (UGameInstance* GI = GetGameInstance())
	//{
	//	if (UGIS_Quest* QuestSys = GI->GetSubsystem<UGIS_Quest>())
	//	{
	//		SaveData.SerializedData = Jsonstring;
	//		QuestSys->ApplySaveData(SaveData);
	//		QuestSys->ApplyQuestData(this);
	//	}
	//}
}

bool UAC_QuestHandleComponent::CheckIfQuestIsCompleted(FName ChoreID)
{
	if (!QuestDataMap.Contains(ChoreID) || !GetOwner())
	{
		return false;
	}

	if (GetChoreStatus(ChoreID) == EChoreStatus::Completed)
	{
		return true;
	}

	for (auto& QuestPart : QuestDataMap[ChoreID].QuestsArray)
	{
		if (QuestPart.bIsCompleted)
		{
			continue;
		}
		bool bAllStepsCompleted = true;

		for (auto& QuestStep : QuestPart.QuestStepDataArray)
		{
			if (QuestStep.bIsChecked)
			{
				continue;
			}

			if (QuestStep.QuestStepTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("WorldMaps"))))
			{
				FGameplayTag CurrentLocationTag = FGameplayTag::EmptyTag;
				if (UGameInstance* GI = GetOwner()->GetGameInstance())
				{
					if (ULocationTrackerSubsystem* TrackerSubSystem = GI->GetSubsystem<ULocationTrackerSubsystem>())
					{
						//CurrentLocationTag = TrackerSubSystem->GetCurrentLocationTag(); 
					}
				}
				if (!CurrentLocationTag.IsValid() || !QuestStep.QuestStepTag.MatchesTag(CurrentLocationTag))
				{
					bAllStepsCompleted = false;
					break;
				}
				if (!QuestStep.bIsDynamic)
				{
					QuestStep.bIsChecked = true;
				}
			}
			else if (QuestStep.QuestStepTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Inventory"))))
			{
				if (UAC_Inventory* InventoryComponent = GetOwner()->GetComponentByClass<UAC_Inventory>())
				{
					if (!InventoryComponent->CheckIfInventoryContainsItem(QuestStep.QuestStepTag, QuestStep.QuestStepValue))
					{
						bAllStepsCompleted = false;
						break;
					}
					else if (!QuestStep.bIsDynamic)
					{
						QuestStep.bIsChecked = true;
					}
				}
				else
				{
					bAllStepsCompleted = false;
					break;
				}
			}
			else if (QuestStep.QuestStepTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("NPC"))))
			{
				// to do when dialogue subsystem is implemented
			}
		}
		if (bAllStepsCompleted)
		{
			QuestPart.bIsCompleted = true;
		}
	}

	for (auto& QuestPart : QuestDataMap[ChoreID].QuestsArray)
	{
		if (!QuestPart.bIsCompleted)
		{
			return false;
		}
	}

	SetChoreStatus(ChoreID, EChoreStatus::Completed);

	return true;
}
