#include "ItemPlacement/A_ChorePlacementZone.h"
#include "ItemPlacement/A_DropZone.h"
#include "ItemPlacement/A_AreaDropZone.h"
#include "ItemPlacement/A_DropZoneGridVisual.h"
#include "PythonContainers/A_InteractableActor.h"
#include "FloorAssignmentComponent.h"
#include "ProceduralMeshComponent.h"
#include "SaveSubsystems/GIS_ChorePlacement.h"
#include "SaveSubsystems/WS_ActorRegistry.h"

AA_ChorePlacementZone::AA_ChorePlacementZone()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SourceZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SourceZoneMesh"));
	FloorAssignmentComponent = CreateDefaultSubobject<UFloorAssignmentComponent>(TEXT("FloorAssignmentComponent"));
	FloorAssignmentComponent->Registrate(EFloorActorType::HeavyFurniture, ESnapshotChannel::None);

	SourceZoneMesh->SetupAttachment(RootComponent);

	SourceZoneMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SourceZoneMesh->SetCollisionResponseToAllChannels(ECR_Overlap);
	SourceZoneMesh->SetRelativeLocation(FVector(0.0f, -200.0f, 0.0f));
}

void AA_ChorePlacementZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Set the material for the SourceZoneMesh
	if (SourceZoneMeshMaterial)
	{
		if (!DMI_SourceZoneMeshMaterial)
		{
			DMI_SourceZoneMeshMaterial = SourceZoneMesh->CreateDynamicMaterialInstance(0, SourceZoneMeshMaterial);
		}
		else
		{
			SourceZoneMesh->SetMaterial(0, DMI_SourceZoneMeshMaterial);
		}
	}

	// AreaDropZone actor
	if (!AreaDropZone && AreaDropZoneClass)
	{
		AreaDropZone = GetWorld()->SpawnActor<AA_AreaDropZone>(AreaDropZoneClass, GetActorTransform());
		if (!IsValid(AreaDropZone->ParentActor))
		{
			AreaDropZone->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
		AreaDropZone->SetActorRelativeLocation(FVector(0.0f, 100.0f, 0.0f));
	}

	// DropZones actors
	if (DropZoneClass && DropZoneGridVisualClass)
	{
		for (FDropZonePlacementZone& DropZonePlacementZone : DropZonePlacementZones)
		{
			// Create the DropZoneGridVisual actor for this placement zone
			FVector DropZonesGridStartLocationOffset{ 0.0f,0.0f,0.0f };
			if (!DropZonePlacementZone.DropZoneGridVisual)
			{
				FActorSpawnParameters Params;
				Params.Owner = this;
				DropZonePlacementZone.DropZoneGridVisual = GetWorld()->SpawnActor<AA_DropZoneGridVisual>(DropZoneGridVisualClass, GetActorTransform(), Params);
				DropZonePlacementZone.DropZoneGridVisual->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			}

			// Count the number of items that can fit in the drop zone based on the item mesh size and the drop zone mesh size
			if (DropZonePlacementZone.ItemClass)
			{
				if (AA_InteractableActor* ItemCDO = DropZonePlacementZone.ItemClass->GetDefaultObject<AA_InteractableActor>())
				{
					if (ItemCDO->StaticMesh && ItemCDO->StaticMesh->GetStaticMesh() && DropZonePlacementZone.DropZoneGridVisual->GetRootComponent())
					{
						DropZonePlacementZone.ItemMeshSize = ItemCDO->StaticMesh->GetStaticMesh()->GetBoundingBox().GetSize() * (1.0f + GapSizeBetweenInPercent);
						FVector DropZoneMeshSize(FVector::ZeroVector);
						if (UStaticMeshComponent* StaticMeshComponent = DropZonePlacementZone.DropZoneGridVisual->FindComponentByClass<UStaticMeshComponent>())
						{
							if (StaticMeshComponent->GetStaticMesh())
							{
								DropZoneMeshSize = StaticMeshComponent->GetStaticMesh()->GetBoundingBox().GetSize() * StaticMeshComponent->GetComponentScale();
							}
						}

						DropZonePlacementZone.ItemsRows = FMath::FloorToInt(DropZoneMeshSize.X / DropZonePlacementZone.ItemMeshSize.X);
						DropZonePlacementZone.ItemsColumns = FMath::FloorToInt(DropZoneMeshSize.Y / DropZonePlacementZone.ItemMeshSize.Y);
						DropZonePlacementZone.DropZoneQuantity = DropZonePlacementZone.ItemsRows * DropZonePlacementZone.ItemsColumns;
					}
				}
				DropZonesGridStartLocationOffset = FVector(DropZonePlacementZone.ItemMeshSize.X / 2.0f, -DropZonePlacementZone.ItemMeshSize.Y / 2.0f, 0.0f); // Adjust the starting location to center the grid of drop zones within the drop zone mesh

				while (DropZonePlacementZone.DropZones.Num() < DropZonePlacementZone.DropZoneQuantity)
				{
					AA_DropZone* DropZone = GetWorld()->SpawnActorDeferred<AA_DropZone>(DropZoneClass, GetActorTransform(), this);
					DropZone->ItemClass = DropZonePlacementZone.ItemClass;
					DropZone->FinishSpawning(GetActorTransform());
					if (!IsValid(DropZone->ParentActor))
					{
						DropZone->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
					}
					DropZonePlacementZone.DropZones.Add(DropZone);
				}
			}
			while (DropZonePlacementZone.DropZones.Num() > DropZonePlacementZone.DropZoneQuantity)
			{
				AA_DropZone* ZoneToDestroy = DropZonePlacementZone.DropZones.Pop();
				ZoneToDestroy->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				ZoneToDestroy->Destroy();
			}

			// Arrange the drop zones in a grid based on the number of rows and columns
			for (int32 Row = 0; Row < DropZonePlacementZone.ItemsRows; ++Row)
			{
				for (int32 Column = 0; Column < DropZonePlacementZone.ItemsColumns; ++Column)
				{
					int32 Index = Row * DropZonePlacementZone.ItemsColumns + Column;
					if (DropZonePlacementZone.DropZones.IsValidIndex(Index) && IsValid(DropZonePlacementZone.DropZones[Index]))
					{
						FVector LocationOffset = FVector(Row * DropZonePlacementZone.ItemMeshSize.X, -Column * DropZonePlacementZone.ItemMeshSize.Y, 0.0f);
						FTransform GridTransform = DropZonePlacementZone.DropZoneGridVisual->GetTransform();
						GridTransform.SetLocation(GridTransform.TransformPositionNoScale(DropZonesGridStartLocationOffset));
						DropZonePlacementZone.DropZones[Index]->SetActorLocation(GridTransform.TransformPositionNoScale(LocationOffset));
						DropZonePlacementZone.DropZones[Index]->SetActorRotation(DropZonePlacementZone.DropZoneGridVisual->GetActorRotation());
					}
				}
			}
		}
	}

	if (IsValid(ParentActor))
	{
		AttachToActor(ParentActor, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
}

void AA_ChorePlacementZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_ChorePlacementZone::BeginPlay()
{
	Super::BeginPlay();

	if (DMI_SourceZoneMeshMaterial)
	{
		DMI_SourceZoneMeshMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.0f);
	}

	SetChoreStatus(bIsAutoAcivated ? EChoreStatus::Active : EChoreStatus::Inactive);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGIS_ChorePlacement* ChoreSys = GI->GetSubsystem<UGIS_ChorePlacement>())
		{
			ChoreSys->AddSaveableObject(this);

			// Delay the application of chore placement data to the next tick to ensure that all actors are properly initialized
			GetWorld()->GetTimerManager().SetTimerForNextTick([this, ChoreSys]()
				{
					ChoreSys->ApplyChorePlacementData(this);
				});
		}
	}
}

void AA_ChorePlacementZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(AutoResultTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AA_ChorePlacementZone::Destroyed()
{
	if (AreaDropZone)
	{
		AreaDropZone->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		AreaDropZone->Destroy();
	}

	for (FDropZonePlacementZone& DropZonePlacementZone : DropZonePlacementZones)
	{
		if (DropZonePlacementZone.DropZoneGridVisual)
		{
			DropZonePlacementZone.DropZoneGridVisual->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			DropZonePlacementZone.DropZoneGridVisual->Destroy();
		}

		for (AA_DropZone* DropZone : DropZonePlacementZone.DropZones)
		{
			if (DropZone)
			{
				DropZone->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				DropZone->Destroy();
			}
		}
		DropZonePlacementZone.DropZones.Empty();
	}

	Super::Destroyed();
}

void AA_ChorePlacementZone::UpdateSourcePlacedItems()
{
	SourcePlacedItems.Empty();
	SourcePlacedItemTags.Empty();

	TArray<FOverlapResult> Overlaps;
	FVector BoxExtent{ 100.0f,100.0f,100.0f };
	BoxExtent *= SourceZoneMesh->GetComponentScale() / 2.0f;
	FCollisionShape Shape = FCollisionShape::MakeBox(BoxExtent);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	GetWorld()->OverlapMultiByChannel(Overlaps, SourceZoneMesh->GetComponentLocation(), SourceZoneMesh->GetComponentQuat(), ECC_WorldDynamic, Shape, Params);

	for (const FOverlapResult& Result : Overlaps)
	{
		if (AA_InteractableActor* InteractableActor = Cast<AA_InteractableActor>(Result.GetActor()))
		{
			FSourceItemData ItemData;
			ItemData.ItemActor = InteractableActor;
			ItemData.ItemDefaultTransform = InteractableActor->GetActorTransform();
			FGuid ItemId;
			if (UFloorAssignmentComponent* FloorAssignmentComp = InteractableActor->FindComponentByClass<UFloorAssignmentComponent>())
			{
				ItemId = FloorAssignmentComp->GetItemId();
			}
			SourcePlacedItems.FindOrAdd(ItemId) = ItemData;
			SourcePlacedItemTags.Add(InteractableActor->Tag);
		}
	}
}

void AA_ChorePlacementZone::SetChoreStatus(EChoreStatus NewStatus)
{
	switch (NewStatus)
	{
	case EChoreStatus::Inactive:
	case EChoreStatus::Active:
	{
		if (AreaDropZone)
		{
			AreaDropZone->bIsActive = NewStatus == EChoreStatus::Active;
		}

		for (FDropZonePlacementZone& DropZonePlacementZone : DropZonePlacementZones)
		{
			for (AA_DropZone* DropZone : DropZonePlacementZone.DropZones)
			{
				if (DropZone)
				{
					DropZone->bIsActive = NewStatus == EChoreStatus::Active;
				}
			}
		}

		GetWorldTimerManager().ClearTimer(AutoResultTimerHandle);
		if (NewStatus == EChoreStatus::Active && ResultBroadcast == EResultBroadcast::Auto)
		{
			GetWorldTimerManager().SetTimer(AutoResultTimerHandle, [this]()
				{
					FChoreResult ChoreResult = GetResultState();
					if (ChoreResult.bResult)
					{
						GetWorldTimerManager().ClearTimer(AutoResultTimerHandle);
						PublicResultState(ChoreResult);
					}
				}, 0.5f, true);
		}
		break;
	}
	case EChoreStatus::Completed:
	{
		GetWorldTimerManager().ClearTimer(AutoResultTimerHandle);
		break;
	}
	case EChoreStatus::Abandoned:
	{
		GetWorldTimerManager().ClearTimer(AutoResultTimerHandle);
		break;
	}
	case EChoreStatus::Failed:
	{
		GetWorldTimerManager().ClearTimer(AutoResultTimerHandle);
		break;
	}
	default:
		break;
	}
}

EChoreStatus AA_ChorePlacementZone::GetChoreStatus() const
{
	return ChoreStatus;
}

void AA_ChorePlacementZone::ResetChorePlacementZone()
{
	for (const auto& [ItemId, ItemData] : SourcePlacedItems)
	{
		ItemData.ItemActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		ItemData.ItemActor->AttachingDropZone = nullptr;
		ItemData.ItemActor->SetActorTransform(ItemData.ItemDefaultTransform);
		ItemData.ItemActor->bIsHeld = false;
		ItemData.ItemActor->bIsAttached = false;

		if (ItemData.ItemActor->Implements<UInteractiveActorInterface>())
		{
			UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(IInteractiveActorInterface::Execute_GetMeshComponent(ItemData.ItemActor));
			if (PrimitiveComponent)
			{
				PrimitiveComponent->SetSimulatePhysics(true);
			}
		}
	}
}

FChoreResult AA_ChorePlacementZone::GetResultState()
{
	FChoreResult ChoreResult;

	// Check AreaDropZone items lists
	AreaDropZone->UpdateInAreaItemsList();
	ChoreResult.SuccessedItems.Append(AreaDropZone->CorrectItemsList);
	ChoreResult.FailedItems.Append(AreaDropZone->WrongItemsList);

	for (const auto& [ItemTag, ItemQuantity] : AreaDropZone->ItemsTags)
	{
		const int32* SuccessedQuantity = ChoreResult.SuccessedItems.Find(ItemTag);
		const int32 Placed = SuccessedQuantity ? *SuccessedQuantity : 0;
		if (Placed < ItemQuantity)
		{
			ChoreResult.MissedItems.FindOrAdd(ItemTag) += ItemQuantity - Placed;
		}
	}

	// Check DropZone items presence
	for (FDropZonePlacementZone& DropZonePlacementZone : DropZonePlacementZones)
	{
		for (AA_DropZone* DropZone : DropZonePlacementZone.DropZones)
		{
			if (DropZone->bIsOccupied)
			{
				++ChoreResult.SuccessedItems.FindOrAdd(DropZone->ItemTag);
			}
			else
			{
				++ChoreResult.MissedItems.FindOrAdd(DropZone->ItemTag);
			}
		}
	}

	switch (ItemBehavior)
	{
	case EWrongItemBehavior::Ignore:
	{
		ChoreResult.FailedItems.Empty();
		// fallthrough: bResult is counted as in CountAsMistake
	}
	case EWrongItemBehavior::CountAsMistake:
	{
		ChoreResult.bResult = ChoreResult.MissedItems.IsEmpty();
		break;
	}
	case EWrongItemBehavior::BlockCompletion:
	{
		ChoreResult.bResult = ChoreResult.MissedItems.IsEmpty() && ChoreResult.FailedItems.IsEmpty();
		break;
	}
	default:
		break;
	}

	return ChoreResult;
}

FGameplayTag AA_ChorePlacementZone::PublicResultState_Implementation(const FChoreResult& ChoreResult)
{
	FString ResultText = FString::Printf(TEXT("Chore result: %s\n"), ChoreResult.bResult ? "Successed" : "Failed");
	ResultText += TEXT("Successed\n");
	ResultText += TEXT("--------------------------\n");
	for (auto& [ItemTag, ItemQuantity] : ChoreResult.SuccessedItems)
	{
		ResultText += FString::Printf(TEXT("%s %d\n"), *ItemTag.ToString(), ItemQuantity);
	}

	ResultText += TEXT("Missed\n");
	ResultText += TEXT("--------------------------\n");
	for (auto& [ItemTag, ItemQuantity] : ChoreResult.MissedItems)
	{
		ResultText += FString::Printf(TEXT("%s %d\n"), *ItemTag.ToString(), ItemQuantity);
	}

	ResultText += TEXT("Failed\n");
	ResultText += TEXT("--------------------------\n");
	for (auto& [ItemTag, ItemQuantity] : ChoreResult.FailedItems)
	{
		ResultText += FString::Printf(TEXT("%s %d\n"), *ItemTag.ToString(), ItemQuantity);
	}

	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, *ResultText);

	return ChoreResult.bResult ? CompletedTag : FailedTag;
}

TArray<FSaveItemData> AA_ChorePlacementZone::CollectChorePlacementSaveData_Implementation()
{
	TArray<FSaveItemData> SaveDataArray;

	for (const auto& [ItemId, ItemData] : SourcePlacedItems)
	{
		FSaveItemData SaveData;
		SaveData.ItemId = ItemId;

		FGuid AttachingDropZoneId;
		if (ItemData.ItemActor->AttachingDropZone)
		{
			if (UFloorAssignmentComponent* FloorAssignmentComp = ItemData.ItemActor->AttachingDropZone->GetComponentByClass<UFloorAssignmentComponent>())
			{
				AttachingDropZoneId = FloorAssignmentComp->GetItemId();
				SaveData.AttachingDropZoneId = AttachingDropZoneId;
			}
		}
		else
		{
			SaveData.ItemTransform = ItemData.ItemActor->GetActorTransform();
			SaveData.bIsHeld = ItemData.ItemActor->bIsHeld;
			SaveData.bIsAttached = ItemData.ItemActor->bIsAttached;
			if (ItemData.ItemActor->Implements<UInteractiveActorInterface>())
			{
				UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(IInteractiveActorInterface::Execute_GetMeshComponent(ItemData.ItemActor));
				if (PrimitiveComponent)
				{
					SaveData.bHasSimulatedPhysics = PrimitiveComponent->IsSimulatingPhysics();
				}
			}
		}

		SaveDataArray.Add(SaveData);
	}

	return SaveDataArray;
}

void AA_ChorePlacementZone::ApplyChorePlacementSaveData_Implementation(const TArray<FSaveItemData>& SaveDataArray)
{
	UWS_ActorRegistry* ActorRegistrySubsystem = GetWorld()->GetSubsystem<UWS_ActorRegistry>();
	if (!ActorRegistrySubsystem)
	{
		return;
	}

	for (const FSaveItemData& SavedData : SaveDataArray)
	{
		AA_InteractableActor* ItemActor = Cast<AA_InteractableActor>(ActorRegistrySubsystem->FindActorByGuid(SavedData.ItemId));
		if (!ItemActor)
		{
			continue;
		}

		UPrimitiveComponent* PrimitiveComponent = nullptr;
		if (ItemActor->Implements<UInteractiveActorInterface>())
		{
			PrimitiveComponent = Cast<UPrimitiveComponent>(IInteractiveActorInterface::Execute_GetMeshComponent(ItemActor));
		}

		AA_DropZone* AttachingDropZone = Cast<AA_DropZone>(ActorRegistrySubsystem->FindActorByGuid(SavedData.AttachingDropZoneId));
		if (AttachingDropZone)
		{
			ItemActor->AttachingDropZone = AttachingDropZone;
			AttachingDropZone->bIsOccupied = true;
			AttachingDropZone->SetMeshMaterialAndState(0, false);

			if (PrimitiveComponent)
			{
				PrimitiveComponent->SetSimulatePhysics(false);
			}
			ItemActor->AttachToActor(AttachingDropZone, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		}
		else
		{
			ItemActor->SetActorTransform(SavedData.ItemTransform);
			ItemActor->bIsHeld = SavedData.bIsHeld;
			ItemActor->bIsAttached = SavedData.bIsAttached;

			if (PrimitiveComponent)
			{
				PrimitiveComponent->SetSimulatePhysics(SavedData.bHasSimulatedPhysics);
			}
		}
	}
}
