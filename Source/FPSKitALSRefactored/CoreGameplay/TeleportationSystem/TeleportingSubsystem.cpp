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
					if (TeleportDestination->IsActiveDestination == false)
						continue;

					if (TeleportDestination->IsInCooldown())
						continue;

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
			FVector Difference = FVector::ZeroVector;
			FVector RootShiftNew;
			float Shift = -10000;

			if (DestinationActor && TeleportingActor)
			{
				FString SlotName = FString(TEXT(""));
				USlotSceneComponent* SlotComponent = nullptr;

				ATeleportDestination* TeleportDestination = Cast<ATeleportDestination>(DestinationActor);
				for (USlotSceneComponent* Slot : TeleportDestination->Slots)
				{
					if (!Slot) continue;

					if (Slot->GetActiveSlot() == false)
					{
						continue;
					}

					if (Slot->IsInCooldown())
					{
						continue;
					}

					FTransform OriginalTransform = TeleportingActor->GetActorTransform();
					TeleportingActor->SetActorRotation(FRotator::ZeroRotator);

					FVector Origin;
					FVector BoxExtent;
					TeleportingActor->GetActorBounds(true, Origin, BoxExtent, true);

					float H1 = BoxExtent.Z * 2.f; // ������ ������

					TeleportingActor->SetActorTransform(OriginalTransform);

					Difference = Origin - TeleportingActorLocation;

					FVector DestinationLocation = Slot->GetComponentLocation();
					FRotator DestinationRotation = Slot->GetComponentRotation();

					FVector RootShift = FVector(0, 0, 1);

					FVector OriginNew;
					FVector BoxExtentNew;
					FRotator BoxRotation;
					//RootShiftNew = Slot->GetComponentTransform().GetRotation().RotateVector(RootShift);
					//Difference = Slot->GetComponentTransform().GetRotation().RotateVector(Difference);
					//float H = 0;

					GetReorientedActorBounds(TeleportingActor, Slot, OriginNew, BoxExtentNew, BoxRotation);

					RootShiftNew = BoxRotation.RotateVector(RootShift);
					Difference = BoxRotation.RotateVector(Difference);

					TArray<AActor*> ActorsToIgnore;
					ActorsToIgnore.Add(DestinationActor);

					FHitResult HitResult;

					

					for (float i = 0; i < H1; i += 0.1f)
					{
						UKismetSystemLibrary::BoxTraceSingle(
							TeleportingActor->GetWorld(),
							DestinationLocation + RootShiftNew * i,
							DestinationLocation + RootShiftNew * i,
							BoxExtent,
							Slot->GetComponentRotation(),
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

							Shift = i + 0.1f;

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
					TeleportingActor->SetActorLocationAndRotation(SlotComponent->GetComponentLocation() + RootShiftNew * Shift - Difference, SlotComponent->GetComponentRotation());

					if (TeleportDestination->GetCoolDownTime() > 0)
					{
						if (!TeleportDestination->IsInCooldown())
						{
							OnDestinationStartCooldown.Broadcast(TeleportDestination);
							TeleportDestination->OnDestinationFinishCooldown.AddDynamic(this, &UTeleportingSubsystem::DestinationFinishCooldown);
							TeleportDestination->StartCooldown();
						}
					}

					if (SlotComponent->GetCoolDownTime() > 0)
					{
						if (!SlotComponent->IsInCooldown())
						{
							OnSlotStartCooldown.Broadcast(TeleportDestination, SlotComponent);
							SlotComponent->OnStopSlotCooldown.AddDynamic(this, &UTeleportingSubsystem::SlotFinishCooldown);
							SlotComponent->StartCooldown();
						}
					}

					OnTeleportation.Broadcast(TeleportDestination, SlotComponent, TeleportingActor);
					UE_LOG(LogTemp, Log, TEXT("Teleported %s to %s slot %s"), *TeleportingActor->GetName(), *DestinationActor->GetName(), *SlotName);
				}
				else
				{
					OnTeleportationFailed.Broadcast(ObjectId, DestinationId);
					UE_LOG(LogTemp, Warning, TEXT("No suitable slot found for Object ID: %s and Destination ID: %s"), *ObjectId, *DestinationId);
				}
				return;
			}
			else
			{
				OnTeleportationFailed.Broadcast(ObjectId, DestinationId);
				UE_LOG(LogTemp, Warning, TEXT("Destination or Teleporting Actor not found for Object ID: %s and Destination ID: %s"), *ObjectId, *DestinationId);
			}
		}
	}

	OnTeleportationFailed.Broadcast(ObjectId, DestinationId);
	UE_LOG(LogTemp, Warning, TEXT("No teleport row found for Object ID: %s and Destination ID: %s"), *ObjectId, *DestinationId);
}

void UTeleportingSubsystem::DestinationFinishCooldown(ATeleportDestination* Destination)
{
	if (Destination)
	{
		Destination->OnDestinationFinishCooldown.Clear();
		OnDestinationFinishCooldown.Broadcast(Destination);
		Destination->OnDestinationFinishCooldown.RemoveDynamic(this, &UTeleportingSubsystem::DestinationFinishCooldown);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("OnDestinationFinishCooldown called with null Destination"));
	}
}

void UTeleportingSubsystem::SlotFinishCooldown(ATeleportDestination* Destination, USlotSceneComponent* Slot)
{
	if (Slot)
	{
		Slot->OnStopSlotCooldown.Clear();
		OnSlotFinishCooldown.Broadcast(Destination, Slot);
		Slot->OnStopSlotCooldown.RemoveDynamic(this, &UTeleportingSubsystem::SlotFinishCooldown);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("OnStopSlotCooldown called with null Slot"));
	}
}
/*
void UTeleportingSubsystem::GetReorientedActorBounds(const AActor* Actor, const USceneComponent* Slot, FVector& OutOrigin, FVector& OutExtent, FRotator& OutRotation)
{
	if (!Actor || !Slot)
	{
		OutOrigin = FVector::ZeroVector;
		OutExtent = FVector::ZeroVector;
		OutRotation = FRotator::ZeroRotator;
		return;
	}

	FVector Origin, BoxExtent;
	Actor->GetActorBounds(true, Origin, BoxExtent);

	// �������� 8 ������ OBB ������ � ��� ������� ����������
	FQuat ActorRotation = Actor->GetActorQuat();
	TArray<FVector> ActorCorners;
	ActorCorners.Add(Origin + ActorRotation.RotateVector(FVector(BoxExtent.X, BoxExtent.Y, BoxExtent.Z)));
	ActorCorners.Add(Origin + ActorRotation.RotateVector(FVector(BoxExtent.X, BoxExtent.Y, -BoxExtent.Z)));
	ActorCorners.Add(Origin + ActorRotation.RotateVector(FVector(BoxExtent.X, -BoxExtent.Y, BoxExtent.Z)));
	ActorCorners.Add(Origin + ActorRotation.RotateVector(FVector(BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z)));
	ActorCorners.Add(Origin + ActorRotation.RotateVector(FVector(-BoxExtent.X, BoxExtent.Y, BoxExtent.Z)));
	ActorCorners.Add(Origin + ActorRotation.RotateVector(FVector(-BoxExtent.X, BoxExtent.Y, -BoxExtent.Z)));
	ActorCorners.Add(Origin + ActorRotation.RotateVector(FVector(-BoxExtent.X, -BoxExtent.Y, BoxExtent.Z)));
	ActorCorners.Add(Origin + ActorRotation.RotateVector(FVector(-BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z)));

	// �������� ���������� �����
	FQuat SlotRotation = Slot->GetComponentQuat();

	// ��������������� �������: ��� ���� �� ����� ��� �������� ��� ����
	TArray<FVector> ReorientedCorners;
	for (const FVector& Corner : ActorCorners)
	{
		// ��������� Corner � ��������� ���������� ������
		FVector Local = ActorRotation.Inverse().RotateVector(Corner - Origin);

		// ������������ ��������� ���������� �� ���������� �����
		FVector Rotated = SlotRotation.RotateVector(Local);

		// ������� ������� � Origin
		ReorientedCorners.Add(Rotated + Origin);
	}

	// ������ ����� FBox
	FBox ReorientedBox(EForceInit::ForceInit);
	for (const FVector& Corner : ReorientedCorners)
	{
		ReorientedBox += Corner;
	}

	OutOrigin = ReorientedBox.GetCenter();
	OutExtent = ReorientedBox.GetExtent();
	OutRotation = SlotRotation.Rotator(); // ���������� �����
}
*/

void UTeleportingSubsystem::GetReorientedActorBounds(const AActor* Actor, const USceneComponent* Slot, FVector& OutOrigin, FVector& OutExtent, FRotator& OutRotation)
{
	if (!Actor || !Slot)
	{
		OutOrigin = FVector::ZeroVector;
		OutExtent = FVector::ZeroVector;
		OutRotation = FRotator::ZeroRotator;
		return;
	}

	// �������� �������� bounding box
	FVector Origin, BoxExtent;
	Actor->GetActorBounds(true, Origin, BoxExtent);

	// ������ ��������� ������� ����� (� ������� ��������� ������ �� ��������)
	TArray<FVector> LocalCorners;
	LocalCorners.Add(FVector(BoxExtent.X, BoxExtent.Y, BoxExtent.Z));
	LocalCorners.Add(FVector(BoxExtent.X, BoxExtent.Y, -BoxExtent.Z));
	LocalCorners.Add(FVector(BoxExtent.X, -BoxExtent.Y, BoxExtent.Z));
	LocalCorners.Add(FVector(BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z));
	LocalCorners.Add(FVector(-BoxExtent.X, BoxExtent.Y, BoxExtent.Z));
	LocalCorners.Add(FVector(-BoxExtent.X, BoxExtent.Y, -BoxExtent.Z));
	LocalCorners.Add(FVector(-BoxExtent.X, -BoxExtent.Y, BoxExtent.Z));
	LocalCorners.Add(FVector(-BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z));

	// ������������ ��������� ������� �� ���������� �����
	FQuat SlotRotation = Slot->GetComponentQuat();
	TArray<FVector> RotatedCorners;
	for (const FVector& Local : LocalCorners)
	{
		RotatedCorners.Add(SlotRotation.RotateVector(Local));
	}

	//FVector SlotZ = Slot->GetComponentQuat().GetAxisZ(); // ��������������� ������
	//float MinZ = FLT_MAX;
	//float MaxZ = -FLT_MAX;

	// ������ ����� FBox
	FBox ReorientedBox(EForceInit::ForceInit);
	for (const FVector& Corner : RotatedCorners)
	{
		ReorientedBox += Corner;

		//float Projection = FVector::DotProduct(Corner, SlotZ);
		//MinZ = FMath::Min(MinZ, Projection);
		//MaxZ = FMath::Max(MaxZ, Projection);

	}

	//H = MaxZ - MinZ; // ������ � ����������� ��� Z �����

	OutOrigin = ReorientedBox.GetCenter() + Origin; // ����� � ����
	OutExtent = ReorientedBox.GetExtent();
	OutRotation = SlotRotation.Rotator();
}
