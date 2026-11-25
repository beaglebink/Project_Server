#include "OnlineOrdering/AC_OnlineOrderingHandle.h"
#include "Engine/AssetManager.h"
#include "CoreGameplay/LocationTrackerSubsystem/LocationTrackerSubsystem.h"
#include "Inventory/AC_Inventory.h"
#include "Utils/GenericJsonLibrary.h"

UAC_OnlineOrderingHandle::UAC_OnlineOrderingHandle()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UAC_OnlineOrderingHandle::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UAC_OnlineOrderingHandle::BeginPlay()
{
	Super::BeginPlay();

	// JUST FOR TEST
	LoadOnlineOrderingData(FName(TEXT("PDA_Order_1")));
	if (!OnlineOrderingDataMap.IsEmpty())
	{
		OnlineOrderingDataMap.begin()->Value.OrderStatus = EChoreStatus::Active;
	}
}

void UAC_OnlineOrderingHandle::EndPlay(const EEndPlayReason::Type Reason)
{

	Super::EndPlay(Reason);
}


bool UAC_OnlineOrderingHandle::LoadOnlineOrderingData(FName OrderName)
{
	const FPrimaryAssetId AssetId(FPrimaryAssetType(TEXT("PDA_OnlineOrdering")), OrderName);
	UAssetManager& AssetManager = UAssetManager::Get();

	TArray<FPrimaryAssetId> RegisteredAssets;

	AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType(TEXT("PDA_OnlineOrdering")), RegisteredAssets);

	const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetId);

	if (!AssetPath.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("OnlineOrdering asset not registered: %s"), *AssetId.ToString());
		return false;
	}

	TSharedPtr<FStreamableHandle> Handle = AssetManager.GetStreamableManager().RequestSyncLoad(AssetPath);

	if (!Handle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load OnlineOrdering asset: %s"), *AssetId.ToString());
		return false;
	}

	UPDA_OnlineOrdering* OrderAsset = Cast<UPDA_OnlineOrdering>(Handle->GetLoadedAsset());

	if (!OrderAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid OnlineOrdering asset: %s"), *AssetId.ToString());
		return false;
	}

	OnlineOrderingDataMap.Add(AssetId.PrimaryAssetName, OrderAsset->OrderData);

	return true;
}

void UAC_OnlineOrderingHandle::SetChoreStatus(FName OrderName, EChoreStatus NewStatus)
{
	if (OnlineOrderingDataMap.Contains(OrderName))
	{
		if (OnlineOrderingDataMap[OrderName].OrderStatus != NewStatus)
		{
			OnlineOrderingDataMap[OrderName].OrderStatus = NewStatus;

			switch (OnlineOrderingDataMap[OrderName].OrderStatus)
			{
			case EChoreStatus::Inactive:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Inactive"), *OrderName.ToString());
				break;
			}
			case EChoreStatus::Active:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Active"), *OrderName.ToString());
				break;
			}
			case EChoreStatus::Completed:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Completed"), *OrderName.ToString());
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Quest %s is now Completed"), *OrderName.ToString()));
				break;
			}
			case EChoreStatus::Failed:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Failed"), *OrderName.ToString());
				break;
			}
			case EChoreStatus::Abandoned:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Abandoned"), *OrderName.ToString());
				break;
			}
			case EChoreStatus::Unavailable:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Unavailable"), *OrderName.ToString());
				break;
			}
			case EChoreStatus::Available:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Available"), *OrderName.ToString());
				break;
			}
			case EChoreStatus::Offered:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Offered"), *OrderName.ToString());
				break;
			}
			case EChoreStatus::Accepted:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Accepted"), *OrderName.ToString());
				break;
			}
			case EChoreStatus::WaitingToStart:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now WaitingToStart"), *OrderName.ToString());
				break;
			}
			case EChoreStatus::Expired:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Expired"), *OrderName.ToString());
				break;
			}
			case EChoreStatus::RetryAvailable:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now RetryAvailable"), *OrderName.ToString());
				break;
			}
			case EChoreStatus::PendingReaccept:
			{
				UE_LOG(LogTemp, Log, TEXT("Quest %s is now PendingReaccept"), *OrderName.ToString());
				break;
			}
			case EChoreStatus::Started:
			{
				if (OnlineOrderingDataMap[OrderName].bWasStarted)
				{
					break;
				}
				OnlineOrderingDataMap[OrderName].bWasStarted = true;

				UE_LOG(LogTemp, Log, TEXT("Quest %s is now Started"), *OrderName.ToString());

				SetChoreStatus(OrderName, EChoreStatus::Active);
				break;
			}
			default:
			{
				UE_LOG(LogTemp, Warning, TEXT("Quest %s has an unknown status"), *OrderName.ToString());
				break;
			}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Quest not found: %s"), *OrderName.ToString());
	}
}

EChoreStatus UAC_OnlineOrderingHandle::GetChoreStatus(FName OrderName)const
{
	if (OnlineOrderingDataMap.Contains(OrderName))
	{
		return OnlineOrderingDataMap[OrderName].OrderStatus;
	}

	return EChoreStatus::Inactive;
}

FString UAC_OnlineOrderingHandle::CollectSaveData()
{
	FOnlineOrderingSaveData OnlineOrderSaveData;
	OnlineOrderSaveData.OnlineOrderingDataMap = OnlineOrderingDataMap;

	return UGenericJsonLibrary::StructToJsonStringCpp(OnlineOrderSaveData);
}

void UAC_OnlineOrderingHandle::ApplySaveData(FString Jsonstring)
{
	FOnlineOrderingSaveData OnlineOrderSaveData;

	if (UGenericJsonLibrary::JsonStringToStructCpp(Jsonstring, OnlineOrderSaveData))
	{
		OnlineOrderingDataMap = OnlineOrderSaveData.OnlineOrderingDataMap;
	}
}

bool UAC_OnlineOrderingHandle::CheckIfOrderIsCompleted(FName OrderName)
{
	return false;
}

FOnlineOrderingData UAC_OnlineOrderingHandle::GetActiveOrder()
{
	for (auto& [OrderName, OrderData] : OnlineOrderingDataMap)
	{
		if (OrderData.OrderStatus == EChoreStatus::Active)
		{
			return OrderData;
		}
	}
	return FOnlineOrderingData();
}
