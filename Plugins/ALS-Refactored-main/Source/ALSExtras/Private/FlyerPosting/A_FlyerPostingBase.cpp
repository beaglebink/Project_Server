#include "FlyerPosting/A_FlyerPostingBase.h"
#include "InteractiveItemComponent.h"
#include "FloorAssignmentComponent.h"
#include "SaveSubsystems/GIS_FlyerPosting.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Components/BoxComponent.h"

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

	if (!MID_Flyer && IsValid(FlyerMaterial))
	{
		MID_Flyer = UMaterialInstanceDynamic::Create(FlyerMaterial, this);
	}
	if (MID_Flyer)
	{
		MID_Flyer->SetTextureParameterValue(TEXT("FontMaskTexture"), TextRenderTarget);
	}
}

void AA_FlyerPostingBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_FlyerPostingBase::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGIS_FlyerPosting* GIS = GI->GetSubsystem<UGIS_FlyerPosting>())
		{
			GIS->AddSaveableObject(this);

			// Delay the application of chore placement data to the next tick to ensure that all actors are properly initialized
			GetWorldTimerManager().SetTimerForNextTick([this, GIS]()
				{
					ApplyFlyerPostingSaveData_Implementation(0);
				});
		}
	}
}

void AA_FlyerPostingBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

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

int32 AA_FlyerPostingBase::CollectFlyerPostingSaveData_Implementation()
{


	return 0;
}

void AA_FlyerPostingBase::ApplyFlyerPostingSaveData_Implementation(const int32 SaveData)
{

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
			ApplyFlyerPostingSaveData_Implementation(0);
		}
	}
}

bool AA_FlyerPostingBase::CheckIfStarted()
{

	return false;
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

void AA_FlyerPostingBase::AddPlacementBox()
{
	UStaticMeshComponent* PlacementMeshComponent = NewObject<UStaticMeshComponent>(this);
	PlacementMeshComponent->SetupAttachment(RootComponent);

	AddInstanceComponent(PlacementMeshComponent);
	PlacementMeshComponent->RegisterComponent();

	if (PlacementStaticMesh && MID_Flyer)
	{
		PlacementMeshComponent->SetStaticMesh(PlacementStaticMesh);
		PlacementMeshComponent->SetMaterial(0, MID_Flyer);
	}
}