#include "FPSKitGameMode.h"
#include "InteriorInstanceSystem/InteriorSubsystem.h"
#include "AlsCharacterExample.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

AFPSKitGameMode::AFPSKitGameMode()
{
	bUseSeamlessTravel = true;
}

void AFPSKitGameMode::GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList)
{
	Super::GetSeamlessTravelActorList(bToTransition, ActorList);

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (PC)
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			ActorList.AddUnique(Pawn);
			UE_LOG(LogTemp, Log, TEXT("FPSKitGameMode: Adding pawn '%s' to seamless travel list"), *Pawn->GetName());
		}
	}
}

void AFPSKitGameMode::NotifyLoadingStarted()
{
	OnLevelLoadingStarted.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("FPSKitGameMode: OnLevelLoadingStarted broadcast"));
}

void AFPSKitGameMode::NotifyLoadingFinished()
{
	OnLevelLoadingFinished.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("FPSKitGameMode: OnLevelLoadingFinished broadcast"));
}

void AFPSKitGameMode::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UInteriorSubsystem* IS = GI->GetSubsystem<UInteriorSubsystem>())
		{
			if (IS->HasPendingSpawnTransform())
			{
				FVector PendingLocation;
				FRotator PendingRotation;
				IS->GetPendingSpawnTransform(PendingLocation, PendingRotation);

				APlayerController* PC = GetWorld()->GetFirstPlayerController();
				if (PC && PC->GetPawn())
				{
					APawn* Pawn = PC->GetPawn();
					Pawn->SetActorLocationAndRotation(
						PendingLocation,
						PendingRotation,
						false,
						nullptr,
						ETeleportType::TeleportPhysics);
					PC->SetControlRotation(PendingRotation);

					UE_LOG(LogTemp, Log, TEXT("FPSKitGameMode::PostSeamlessTravel - Pawn '%s' moved to %s"),
						*Pawn->GetName(), *PendingLocation.ToString());
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("FPSKitGameMode::PostSeamlessTravel - Pawn not found after travel"));
				}

				IS->ClearPendingSpawnTransform();
			}
		}
	}

	NotifyLoadingFinished();
}