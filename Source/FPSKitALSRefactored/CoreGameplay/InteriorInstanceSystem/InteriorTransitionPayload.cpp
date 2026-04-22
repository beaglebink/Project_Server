#include "InteriorTransitionPayload.h"
#include "../LocationSystem/FloorAsset.h"
#include "../LocationSystem/WorldRegionAsset.h"
#include "../LocationSystem/StreetAsset.h"
#include "../LocationSystem/InteriorSetAsset.h"
#include "../LocationSystem/LocationAnchorActor.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/Engine.h"

FString UInteriorTransitionPayload::GetTargetLevelPackageName() const
{
	// Приоритет: FloorLevel > RegionLevel
	if (!DestinationLink.TargetFloor.IsNull())
	{
		if (UFloorAsset* Floor = DestinationLink.TargetFloor.LoadSynchronous())
		{
			if (!Floor->FloorLevel.IsNull())
				return Floor->FloorLevel.ToSoftObjectPath().GetLongPackageName();
		}
	}

	if (!DestinationLink.TargetRegion.IsNull())
	{
		if (UWorldRegionAsset* Region = DestinationLink.TargetRegion.LoadSynchronous())
		{
			if (!Region->RegionLevel.IsNull())
				return Region->RegionLevel.ToSoftObjectPath().GetLongPackageName();
		}
	}

	return FString();
}

UInteriorTransitionPayload* UInteriorTransitionPayload::SetupFromDescriptor(const FInteriorTransitionDescriptor& Descriptor)
{
	// Сбрасываем
	DestinationLink = FLocationAnchorLink();

	// Запоминаем этаж-источник (с которого уходим)
	SourceFloor = Descriptor.SourceFloor;

	// Сначала копируем иерархию (если указана)
	if (!Descriptor.TargetFloor.IsNull())
	{
		DestinationLink.TargetFloor = Descriptor.TargetFloor;
	}
	if (!Descriptor.TargetRegion.IsNull())
	{
		DestinationLink.TargetRegion = Descriptor.TargetRegion;
	}
	if (!Descriptor.TargetStreet.IsNull())
	{
		DestinationLink.TargetStreet = Descriptor.TargetStreet;
	}
	if (!Descriptor.TargetInteriorSet.IsNull())
	{
		DestinationLink.TargetInteriorSet = Descriptor.TargetInteriorSet;
	}

	// Если задан TransitionPointId — сохраним его в AnchorID (логика поиска места может учитывать TP)
	// Здесь мы предпочитаем явный TargetAnchorID, если он есть.
	if (Descriptor.TargetAnchorID.IsValid())
	{
		DestinationLink.TargetAnchorID = Descriptor.TargetAnchorID;
	}
	else if (Descriptor.TransitionPointId.IsValid())
	{
		// Пробуем разрешить TransitionPoint -> AnchorID по ассету (если указан этаж)
		if (!DestinationLink.TargetFloor.IsNull())
		{
			if (UFloorAsset* Floor = DestinationLink.TargetFloor.LoadSynchronous())
			{
				for (const FLocationTransitionPoint& TP : Floor->TransitionPoints)
				{
					if (TP.TransitionPointID == Descriptor.TransitionPointId)
					{
						// Ранее использовалось TP.DestinationLocationID (устарело) — используем новую модель
						DestinationLink.TargetAnchorID = TP.DestinationLink.TargetAnchorID;
						break;
					}
				}
			}
		}
	}
	// Если есть AnchorIndex — попытка взять из Floor->Anchors
	if (!DestinationLink.TargetAnchorID.IsValid() && Descriptor.AnchorIndex >= 0)
	{
		if (!DestinationLink.TargetFloor.IsNull())
		{
			if (UFloorAsset* Floor = DestinationLink.TargetFloor.LoadSynchronous())
			{
				if (Floor->Anchors.IsValidIndex(Descriptor.AnchorIndex))
				{
					DestinationLink.TargetAnchorID = Floor->Anchors[Descriptor.AnchorIndex].AnchorID;
				}
			}
		}
	}

	// Если указан AnchorName — искать по DisplayName или по GameplayTag
	if (!DestinationLink.TargetAnchorID.IsValid() && Descriptor.AnchorName != NAME_None)
	{
		const FString AnchorNameStr = Descriptor.AnchorName.ToString();
		// Сначала по Floor
		if (!DestinationLink.TargetFloor.IsNull())
		{
			if (UFloorAsset* Floor = DestinationLink.TargetFloor.LoadSynchronous())
			{
				for (const FLocationAnchor& A : Floor->Anchors)
				{
					if (!A.DisplayName.IsEmpty() && A.DisplayName.ToString().Equals(AnchorNameStr, ESearchCase::IgnoreCase))
					{
						DestinationLink.TargetAnchorID = A.AnchorID;
						break;
					}
					if (!A.Tags.IsEmpty())
					{
						const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(Descriptor.AnchorName);
						if (Tag.IsValid() && A.Tags.HasTagExact(Tag))
						{
							DestinationLink.TargetAnchorID = A.AnchorID;
							break;
						}
					}
				}
			}
		}
		// Если не нашли — по Region
		if (!DestinationLink.TargetAnchorID.IsValid() && !DestinationLink.TargetRegion.IsNull())
		{
			if (UWorldRegionAsset* Region = DestinationLink.TargetRegion.LoadSynchronous())
			{
				for (const FLocationAnchor& A : Region->Anchors)
				{
					if (!A.DisplayName.IsEmpty() && A.DisplayName.ToString().Equals(AnchorNameStr, ESearchCase::IgnoreCase))
					{
						DestinationLink.TargetAnchorID = A.AnchorID;
						break;
					}
					if (!A.Tags.IsEmpty())
					{
						const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(Descriptor.AnchorName);
						if (Tag.IsValid() && A.Tags.HasTagExact(Tag))
						{
							DestinationLink.TargetAnchorID = A.AnchorID;
							break;
						}
					}
				}
			}
		}
	}

	// Попытка заполнить DisplayName по ассетам, если есть TargetAnchorID
	DestinationLink.TargetAnchorDisplayName = FText::GetEmpty();
	if (DestinationLink.TargetAnchorID.IsValid())
	{
		// По этажу
		if (!DestinationLink.TargetFloor.IsNull())
		{
			if (UFloorAsset* Floor = DestinationLink.TargetFloor.LoadSynchronous())
			{
				for (const FLocationAnchor& A : Floor->Anchors)
				{
					if (A.AnchorID == DestinationLink.TargetAnchorID)
					{
						DestinationLink.TargetAnchorDisplayName = A.DisplayName;
						break;
					}
				}
			}
		}
		// По региону (fallback)
		if (DestinationLink.TargetAnchorDisplayName.IsEmpty() && !DestinationLink.TargetRegion.IsNull())
		{
			if (UWorldRegionAsset* Region = DestinationLink.TargetRegion.LoadSynchronous())
			{
				for (const FLocationAnchor& A : Region->Anchors)
				{
					if (A.AnchorID == DestinationLink.TargetAnchorID)
					{
						DestinationLink.TargetAnchorDisplayName = A.DisplayName;
						break;
					}
				}
			}
		}
	}

	return this;
}

// Простой Setup: копируем готовую ссылку (используется, например, ALocationAnchorActor::CreateTransitionPayload)
UInteriorTransitionPayload* UInteriorTransitionPayload::Setup(const FLocationAnchorLink& InDestinationLink)
{
    DestinationLink = InDestinationLink;
    return this;
}