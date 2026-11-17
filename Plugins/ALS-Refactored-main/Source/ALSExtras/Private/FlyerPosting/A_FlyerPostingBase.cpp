#include "FlyerPosting/A_FlyerPostingBase.h"
#include "InteractiveItemComponent.h"
#include "FloorAssignmentComponent.h"
#include "SaveSubsystems/GIS_FlyerPosting.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Inventory/AC_Inventory.h"
#include "InteractivePickerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "FlyerPosting/A_Flyer.h"

AA_FlyerPostingBase::AA_FlyerPostingBase()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	InteractiveItemComponent = CreateDefaultSubobject<UInteractiveItemComponent>(TEXT("InteractiveItemComponent"));
	FloorAssignmentComponent = CreateDefaultSubobject<UFloorAssignmentComponent>(TEXT("FloorAssignmentComponent"));
}

void AA_FlyerPostingBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!TextRenderTarget)
	{
		TextRenderTarget = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(this, UCanvasRenderTarget2D::StaticClass(), 128, 128);
		TextRenderTarget->OnCanvasRenderTargetUpdate.AddDynamic(this, &AA_FlyerPostingBase::DrawZoneText);
	}
	if (TextRenderTarget)
	{
		TextRenderTarget->UpdateResource();
	}

	if (!MID_FlyerZone && IsValid(FlyerZoneMaterial))
	{
		MID_FlyerZone = UMaterialInstanceDynamic::Create(FlyerZoneMaterial, this);
	}
	if (MID_FlyerZone)
	{
		MID_FlyerZone->SetTextureParameterValue(TEXT("FontMaskTexture"), TextRenderTarget);
	}
}

void AA_FlyerPostingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_FlyerPostingBase::BeginPlay()
{
	Super::BeginPlay();

	if (bIsAutoAcivated)
	{
		SetChoreStatus(EChoreStatus::Active);
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGIS_FlyerPosting* GIS = GI->GetSubsystem<UGIS_FlyerPosting>())
		{
			GIS->AddSaveableObject(this);

			// Delay the application of chore placement data to the next tick to ensure that all actors are properly initialized
			GetWorldTimerManager().SetTimerForNextTick([this, GIS]()
				{
					ApplyFlyerPostingSaveData_Implementation(GIS);
				});
		}
	}

	InteractiveItemComponent->OnInteractionPressKeyEvent.AddDynamic(this, &AA_FlyerPostingBase::OnInteract);
	InteractiveItemComponent->OnInteractiveReceiveFocusEvent.AddDynamic(this, &AA_FlyerPostingBase::OnInteractiveReceiveFocus);
	InteractiveItemComponent->OnInteractiveLostFocusEvent.AddDynamic(this, &AA_FlyerPostingBase::OnInteractiveLostFocus);
}

void AA_FlyerPostingBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(TooltipCheckHandle);

	Super::EndPlay(EndPlayReason);
}

void AA_FlyerPostingBase::Destroyed()
{

	Super::Destroyed();
}

void AA_FlyerPostingBase::SetChoreStatus(EChoreStatus NewStatus)
{
	if (ChoreStatus == NewStatus)
	{
		return;
	}
	ChoreStatus = NewStatus;

	switch (ChoreStatus)
	{
	case EChoreStatus::Inactive:
	case EChoreStatus::Active:
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, ChoreStatus == EChoreStatus::Active ? "ACTIVE" : "INACTIVE");
		break;
	}
	case EChoreStatus::Completed:
	{

		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "COMPLETED");
		break;
	}
	case EChoreStatus::Failed:
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "FAILED");
		break;
	}
	case EChoreStatus::Abandoned:
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "ABANDONED");
		break;
	}
	case EChoreStatus::Unavailable:
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "UNAVAILABLE");
		break;
	}
	case EChoreStatus::Available:
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "AVAILABLE");
		break;
	}
	case EChoreStatus::Offered:
	{

		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "OFFERED");
		break;
	}
	case EChoreStatus::Accepted:
	{

		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "ACCEPTED");
		break;
	}
	case EChoreStatus::WaitingToStart:
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "WAITINGTOSTART");
		break;
	}
	case EChoreStatus::Expired:
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "EXPIRED");
		break;
	}
	case EChoreStatus::RetryAvailable:
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "RETRYAVAILABLE");
		break;
	}
	case EChoreStatus::PendingReaccept:
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "PENDINGREACCEPT");
		break;
	}
	case EChoreStatus::Started:
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, "STARTED");
		OnStarted();
		break;
	}
	default:
		break;
	}
}

EChoreStatus AA_FlyerPostingBase::GetChoreStatus() const
{
	return ChoreStatus;
}

FGameplayTag AA_FlyerPostingBase::PublicResultState_Implementation(bool Result)
{

	return Result ? CompletedTag : FailedTag;
}

FString AA_FlyerPostingBase::GetFloorAssignmentComponentIdAsString_Implementation()
{
	if (IsValid(FloorAssignmentComponent))
	{
		return FloorAssignmentComponent->GetFloorId().ToString();
	}
	return FString();
}

FFlyerPostingSaveData AA_FlyerPostingBase::CollectFlyerPostingSaveData_Implementation()
{
	FFlyerPostingSaveData FlyerPostingSaveData;
	FlyerPostingSaveData.ChoreStatus = GetChoreStatus();
	FlyerPostingSaveData.PlacedFlyers = PlacedFlyers;

	return FFlyerPostingSaveData();
}

void AA_FlyerPostingBase::ApplyFlyerPostingSaveData_Implementation(UGIS_FlyerPosting* GIS)
{
	FFlyerPostingSaveData* FlyerPostingSaveData = GIS->CachedFlyerPostingData.Find(GetFloorAssignmentComponentIdAsString_Implementation());
	if (FlyerPostingSaveData)
	{
		SetChoreStatus(FlyerPostingSaveData->ChoreStatus);
		PlacedFlyers = FlyerPostingSaveData->PlacedFlyers;
		for (const auto& [FlyerId, FlyerTransform] : PlacedFlyers)
		{
			SpawnFlyer(FlyerId, FlyerTransform);
		}
	}
}

FString AA_FlyerPostingBase::CollectIntermediateSaveData()
{
	FSubsystemSaveData SaveData;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGIS_FlyerPosting* GIS = GI->GetSubsystem<UGIS_FlyerPosting>())
		{
			GIS->CollectSaveData(SaveData);
		}
	}
	return SaveData.SerializedData;
}

void AA_FlyerPostingBase::ApplyIntermediateSaveData(FString Jsonstring)
{
	FSubsystemSaveData SaveData;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGIS_FlyerPosting* GIS = GI->GetSubsystem<UGIS_FlyerPosting>())
		{
			SaveData.SerializedData = Jsonstring;
			GIS->ApplySaveData(SaveData);
			ApplyFlyerPostingSaveData_Implementation(GIS);
		}
	}
}

bool AA_FlyerPostingBase::CheckIfStarted()
{

	return false;
}

void AA_FlyerPostingBase::OnInteract(UInteractivePickerComponent* Picker)
{
	if (GetChoreStatus() == EChoreStatus::Inactive || GetChoreStatus() == EChoreStatus::Completed)
	{
		return;
	}

	if (Picker->GetOwner())
	{
		if (ACharacter* InteractingCharacter = Cast<ACharacter>(Picker->GetOwner()))
		{
			if (UAC_Inventory* InventoryComp = InteractingCharacter->FindComponentByClass<UAC_Inventory>())
			{
				FHitResult HitResult = TraceForFlyer();
				if (HitResult.bBlockingHit)
				{
					if (AA_Flyer* FlyerActor = Cast<AA_Flyer>(HitResult.GetActor()))
					{
						InventoryComp->AddItemToInventory(FlyerActor->FlyerTag, 1);
						PlacedFlyers.Remove(FlyerActor->FlyerId);
						FlyerActor->Destroy();
						FlyersSpawnObserver(false);
					}
					else if (InventoryComp->CheckIfInventoryContainsItem(FlyerTag, 1) && FlyerClass)
					{
						InventoryComp->RemoveItemFromInventory(FlyerTag, 1);
						FTransform FlyerTransform;
						FlyerTransform.SetLocation(HitResult.Location);
						FlyerTransform.SetRotation(HitResult.Normal.ToOrientationQuat());
						FGuid FlyerId = CreateGuidForFlyer();
						PlacedFlyers.Add(FlyerId, FlyerTransform);

						SpawnFlyer(FlyerId, FlyerTransform);
					}
				}
			}
		}
	}
}

void AA_FlyerPostingBase::OnInteractiveReceiveFocus(AActor* WhoInteract)
{
	if (GetChoreStatus() == EChoreStatus::Completed)
	{
		InteractiveItemComponent->SetTooltip(FText::FromString("Flyer posting is completed"));
		return;
	}

	GetWorldTimerManager().ClearTimer(TooltipCheckHandle);
	GetWorldTimerManager().SetTimer(TooltipCheckHandle, [this, WhoInteract]()
		{
			if (GetChoreStatus() == EChoreStatus::Inactive)
			{
				InteractiveItemComponent->SetTooltip(FText::FromString("Flyer posting is inactive"));
				return;
			}

			FHitResult HitResult = TraceForFlyer();
			if (HitResult.bBlockingHit)
			{
				if (AA_Flyer* FlyerActor = Cast<AA_Flyer>(HitResult.GetActor()))
				{
					InteractiveItemComponent->SetTooltip(FText::FromString("The place is taken by another flyer, press F to take it."));
					return;
				}
			}

			if (ACharacter* InteractingCharacter = Cast<ACharacter>(WhoInteract))
			{
				if (UAC_Inventory* InventoryComp = InteractingCharacter->FindComponentByClass<UAC_Inventory>())
				{
					if (InventoryComp->CheckIfInventoryContainsItem(FlyerTag, 1))
					{
						InteractiveItemComponent->SetTooltip(FText::FromString("Press F to place the flyer"));
					}
					else
					{
						InteractiveItemComponent->SetTooltip(FText::FromString("You don't have enough or such flyers"));
					}
				}
			}
		}, 0.5, true, 0.0f);
}

void AA_FlyerPostingBase::OnInteractiveLostFocus(ACharacter* User)
{
	GetWorldTimerManager().ClearTimer(TooltipCheckHandle);
}

void AA_FlyerPostingBase::DrawZoneText(UCanvas* Canvas, int32 Width, int32 Height)
{
	if (!Canvas || !GEngine)
	{
		return;
	}

	FCanvasTextItem TextItem(FVector2D(10.0f, 50.0f), FText::FromName(ChoreId), GEngine->GetLargeFont(), FLinearColor::White);
	Canvas->DrawItem(TextItem);

	Canvas->K2_DrawBox(FVector2D(0.0f, 0.0f), FVector2D(Width, Height), 1.0f, FLinearColor::White);
}

FGuid AA_FlyerPostingBase::CreateGuidForFlyer()
{
	FGuid NewGuid;
	do
	{
		NewGuid = FGuid::NewGuid();
	} while (PlacedFlyers.Contains(NewGuid));

	return NewGuid;
}

void AA_FlyerPostingBase::SpawnFlyer(FGuid FlyerId, FTransform FlyerTransform)
{
	SetChoreStatus(EChoreStatus::Started);

	if (AA_Flyer* NewFlyer = GetWorld()->SpawnActorDeferred<AA_Flyer>(FlyerClass, FlyerTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
	{
		NewFlyer->FlyerId = FlyerId;
		NewFlyer->FlyerTag = FlyerTag;
		NewFlyer->FinishSpawning(FlyerTransform);

		FlyersSpawnObserver(true);
	}
}

FHitResult AA_FlyerPostingBase::TraceForFlyer()
{
	FHitResult HitResult;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

		FCollisionQueryParams Params;
		Params.AddIgnoredActor(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

		GetWorld()->LineTraceSingleByChannel(HitResult, ViewLocation, ViewLocation + ViewRotation.Vector() * (InteractiveItemComponent->InteractionRange + 50.0f), ECC_Visibility, Params);
	}
	return HitResult;
}

FTransform AA_FlyerPostingBase::GetFlyerTransform() const
{
	return FTransform();
}

void AA_FlyerPostingBase::FlyersSpawnObserver(bool IsSpawned)
{
	if (IsSpawned)
	{

		--RequiredFlyersQuantityToPlace;
		if (RequiredFlyersQuantityToPlace == 0)
		{
			if (!bIsDeferredEvaluate)
			{
				SetChoreStatus(EChoreStatus::Completed);
			}
			PublicResultState(true);
		}
	}
	else
	{
		++RequiredFlyersQuantityToPlace;
		SetChoreStatus(EChoreStatus::Active);
	}
}

void AA_FlyerPostingBase::AddPlacementBox()
{
	UStaticMeshComponent* PlacementMeshComponent = NewObject<UStaticMeshComponent>(this);
	PlacementMeshComponent->SetupAttachment(RootComponent);

	AddInstanceComponent(PlacementMeshComponent);
	PlacementMeshComponent->RegisterComponent();

	PlacementMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PlacementMeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
	PlacementMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	PlacementMeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);

	//PlacementMeshComponent->SetHiddenInGame(true);

	if (PlacementStaticMesh && MID_FlyerZone)
	{
		PlacementMeshComponent->SetStaticMesh(PlacementStaticMesh);
		PlacementMeshComponent->SetMaterial(0, MID_FlyerZone);
	}
}