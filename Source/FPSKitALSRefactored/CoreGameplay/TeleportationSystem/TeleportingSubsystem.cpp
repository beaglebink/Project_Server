#include "TeleportingSubsystem.h"
#include "SceneDataProvider.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "TeleportDestination.h"
#include "TeleportingComponent.h"
#include "SlotSceneComponent.h"
#include <Kismet/KismetSystemLibrary.h>

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

	LoadedSceneTable = ISceneDataProvider::Execute_TeleportDataTable(GI);


	if (!LoadedSceneTable || !LoadedSceneTable->RowStruct)
	{
		UE_LOG(LogTemp, Error, TEXT("Scene DataTable is invalid or missing RowStruct"));
		return;
	}

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
	UE_LOG(LogTemp, Log, TEXT("UTeleportingSubsystem::TeleportToDestination ObjectId %s DestinationId %s"), *ObjectId, *DestinationId);

	if (!LoadedSceneTable)
	{
		UE_LOG(LogTemp, Log, TEXT("DataTable not loaded, cannot teleport"));
		return;
	}

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
					UE_LOG(LogTemp, Log, TEXT("UTeleportingSubsystem::TeleportToDestination DestinationActor %s"), *DestinationActor->GetName());
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
						UE_LOG(LogTemp, Log, TEXT("UTeleportingSubsystem::TeleportToDestination TeleportingActor %s"), *TeleportingActor->GetName());
					}
				}
			}

			FVector TeleportingActorLocation = TeleportingActor->GetActorLocation();

			if (DestinationActor && TeleportingActor)
			{
				FString SlotName = FString(TEXT(""));
				USlotSceneComponent* SlotComponent = nullptr;

				ATeleportDestination* TeleportDestination = Cast<ATeleportDestination>(DestinationActor);
				for (USlotSceneComponent* Slot : TeleportDestination->Slots)
				{
					if (!Slot) continue;

					FVector Origin;
					FVector BoxExtent;
					TeleportingActor->GetActorBounds(true, Origin, BoxExtent, true);

					FVector DestinationLocation = Slot->GetComponentLocation();
					FRotator DestinationRotation = Slot->GetComponentRotation();

					FVector RootShift = FVector(0, 0, 1);

					FVector OriginNew;
					FVector BoxExtentNew;
					FRotator BoxRotation;


					RelativeReorientation(TeleportingActor, Slot, OriginNew, BoxExtentNew, BoxRotation);

					TArray<AActor*> ActorsToIgnore;
					ActorsToIgnore.Add(DestinationActor);

					FHitResult HitResult;

					float Shift = -10000;

					for (float i = 0; i < BoxExtentNew.Z * 2; i += 0.1f)
					{
						UKismetSystemLibrary::BoxTraceSingle(
							TeleportingActor->GetWorld(),
							DestinationLocation + RootShift * i,
							DestinationLocation + RootShift * i,
							BoxExtentNew,
							BoxRotation,
							UEngineTypes::ConvertToTraceType(ECC_Visibility),
							false,
							ActorsToIgnore,
							EDrawDebugTrace::None,
							HitResult,
							false, 
							FLinearColor::Red,
							FLinearColor::Green,
							10.f
						);

						if (!HitResult.bBlockingHit)
						{
							SlotName = Slot->SlotName.ToString();
							SlotComponent = Slot;

							Shift = i;

							break;
						}
					}

					if (Shift >= 0)
					{
						break;
					}
				}

				if (SlotComponent)
				{
					TeleportingActor->SetActorLocationAndRotation(SlotComponent->GetComponentLocation(), SlotComponent->GetComponentRotation());
					UE_LOG(LogTemp, Log, TEXT("Teleported %s to %s slot %s"), *TeleportingActor->GetName(), *DestinationActor->GetName(), *SlotName);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("No suitable slot found for Object ID: %s and Destination ID: %s"), *ObjectId, *DestinationId);
				}
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

void UTeleportingSubsystem::RelativeReorientation(const AActor* TargetActor, USceneComponent* TeleportSlot, FVector& OutOrigin, FVector& OutExtent, FRotator& OutRotation)
{
	if (!TargetActor || !TeleportSlot)
	{
		OutOrigin = FVector::ZeroVector;
		OutExtent = FVector::ZeroVector;
		OutRotation = FRotator::ZeroRotator;
		return;
	}

	USceneComponent* RootComponent = TargetActor->GetRootComponent();
	if (!RootComponent)
	{
		OutOrigin = FVector::ZeroVector;
		OutExtent = FVector::ZeroVector;
		OutRotation = FRotator::ZeroRotator;
		return;
	}

	// Получаем локальный Transform RootComponent относительно актора
	const FTransform LocalTransform = RootComponent->GetComponentTransform();

	// Получаем Transform слота
	const FTransform SlotTransform = TeleportSlot->GetComponentTransform();

	// Виртуальный ComponentToWorld: как если бы RootComponent был прикреплён к слоту
	const FTransform VirtualTransform = LocalTransform * SlotTransform;

	// Вычисляем Bounds
	const FBoxSphereBounds VirtualBounds = RootComponent->CalcBounds(VirtualTransform);

	OutOrigin = VirtualBounds.Origin;
	OutExtent = VirtualBounds.BoxExtent;
	OutRotation = VirtualTransform.GetRotation().Rotator(); // ориентация бокса
}