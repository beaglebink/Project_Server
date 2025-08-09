#include "TeleportingSubsystem.h"
#include "SceneDataProvider.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "TeleportDestination.h"
#include "TeleportingComponent.h"

void UTeleportingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Warning, TEXT("TeleportingSubsystem: GameInstance not found"));
        return;
    }

    if (!GI->Implements<USceneDataProvider>())
    {
        UE_LOG(LogTemp, Warning, TEXT("GameInstance does not implement SceneDataProvider"));
        return;
    }

    TSoftObjectPtr<UDataTable> TablePtr = ISceneDataProvider::Execute_GetLevelDataTable(GI);
    if (!TablePtr.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Scene DataTable is not loaded, loading synchronously"));
        TablePtr.LoadSynchronous();
    }

    LoadedSceneTable = TablePtr.Get();
    if (LoadedSceneTable)
    {
        UE_LOG(LogTemp, Log, TEXT("Scene DataTable loaded: %s"), *LoadedSceneTable->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load Scene DataTable"));
    }

}

void UTeleportingSubsystem::Deinitialize()
{
    //LoadedTeleportTable = nullptr;
    Super::Deinitialize();
}

void UTeleportingSubsystem::RegistrationTeleportingDestination(AActor* Destination)
{
    if (Destination && !TeleportingDestinations.Contains(Destination))
    {
        TeleportingDestinations.Add(Destination);
    }
}

void UTeleportingSubsystem::UnregistrationTeleportingDestination(AActor* Destination)
{
    if (Destination && TeleportingDestinations.Contains(Destination))
    {
        TeleportingDestinations.Remove(Destination);
    }
}

void UTeleportingSubsystem::RegistrationTeleportingActor(AActor* Actor)
{
	if (Actor && !TeleportingActors.Contains(Actor))
	{
		TeleportingActors.Add(Actor);
	}
}

void UTeleportingSubsystem::UnregistrationTeleportingActor(AActor* Actor)
{
	if (Actor && TeleportingActors.Contains(Actor))
	{
		TeleportingActors.Remove(Actor);
	}
}

void UTeleportingSubsystem::TeleportToDestination(FString ObjectId, FString DestinationId)
{
	TArray<FName> RowNames = LoadedSceneTable->GetRowNames();
	AActor* DestinationActor = nullptr;
	AActor* TeleportingActor = nullptr;
	for (const FName& RowName : RowNames)
	{
		const FTeleportTableRow* Row = LoadedSceneTable->FindRow<FTeleportTableRow>(RowName, TEXT("TeleportingSubsystem Row Lookup"));
		if (Row && Row->ObjectID == ObjectId && Row->DestinationID == DestinationId)
		{
			for (AActor* Destination : TeleportingDestinations)
			{
				ATeleportDestination* TeleportDestination = Cast<ATeleportDestination>(Destination);
				if (TeleportDestination && TeleportDestination->DestinationID == DestinationId)
				{
					DestinationActor = TeleportDestination;
				}
			}
            
			for (AActor* Actor : TeleportingActors)
			{
				if (Actor)
				{
					UTeleportingComponent* TeleportingComponent = Actor->FindComponentByClass<UTeleportingComponent>();
					if (TeleportingComponent && TeleportingComponent->ObjectID == ObjectId)
					{
						TeleportingActor = Actor;
					}
				}
			}

			if (DestinationActor && TeleportingActor)
			{
				FVector DestinationLocation = DestinationActor->GetActorLocation();
				FRotator DestinationRotation = DestinationActor->GetActorRotation();
				TeleportingActor->SetActorLocationAndRotation(DestinationLocation, DestinationRotation);
				UE_LOG(LogTemp, Log, TEXT("Teleported %s to %s"), *TeleportingActor->GetName(), *DestinationActor->GetName());
				return;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Destination or Teleporting Actor not found for Object ID: %s and Destination ID: %s"), *ObjectId, *DestinationId);
			}
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("No teleport row found for Object ID: %s and Destination ID: %s"), *ObjectId, *DestinationId);
}
