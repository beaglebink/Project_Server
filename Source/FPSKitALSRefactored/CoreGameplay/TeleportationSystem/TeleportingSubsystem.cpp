#include "TeleportingSubsystem.h"
#include "SceneDataProvider.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "TeleportDestination.h"
#include "TeleportingComponent.h"
#include "SlotSceneComponent.h"
#include <Kismet/KismetSystemLibrary.h>
#include <NiagaraFunctionLibrary.h>

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
	TeleportFailResponses.Empty();

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
				if (!Destination)
				{
					UE_LOG(LogTemp, Warning, TEXT("UTeleportingSubsystem::TeleportToDestination Destination is null"));	
					continue;
				}

				ATeleportDestination* TeleportDestination = Cast<ATeleportDestination>(Destination);
				if (TeleportDestination && TeleportDestination->DestinationID == DestinationId)
				{
					if (TeleportDestination->IsActiveDestination == false)
					{
						UTeleportFailResponseObject* FailResponse = NewObject<UTeleportFailResponseObject>(this);
						FailResponse->ObjectId = ObjectId;
						FailResponse->DestinationId = DestinationId;
						FailResponse->Response = FString(TEXT("Destination ")) + DestinationId + FString(TEXT(" is not active"));
						TeleportFailResponses.Add(FailResponse);
						UE_LOG(LogTemp, Warning, TEXT("UTeleportingSubsystem::TeleportToDestination Destination is not active"));

						continue;
					}

					if (TeleportDestination->IsTeleportBusy())
					{
						continue;
					}
						
					if (TeleportDestination->IsInCooldown())
					{
						UTeleportFailResponseObject* FailResponse = NewObject<UTeleportFailResponseObject>(this);
						FailResponse->ObjectId = ObjectId;
						FailResponse->DestinationId = DestinationId;
						FailResponse->Response = FString(TEXT("Destination ")) + DestinationId + FString(TEXT(" is in cooldown"));
						TeleportFailResponses.Add(FailResponse);
						UE_LOG(LogTemp, Warning, TEXT("UTeleportingSubsystem::TeleportToDestination Destination is in cooldown"));
						continue;
					}

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
						if (TeleportingComponent->ActorIsFree)
						{
							TeleportingActor = Actor;
							UE_LOG(LogTemp, Log, TEXT("UTeleportingSubsystem::TeleportToDestination TeleportingActor %s"), *TeleportingActor->GetName());
						}
						else
						{
							UTeleportFailResponseObject* FailResponse = NewObject<UTeleportFailResponseObject>(this);
							FailResponse->ObjectId = ObjectId;
							FailResponse->DestinationId = DestinationId;
							FailResponse->Response = FString(TEXT("Actor ")) + ObjectId + FString(TEXT(" is not free for teleportation"));
							TeleportFailResponses.Add(FailResponse);
							UE_LOG(LogTemp, Warning, TEXT("UTeleportingSubsystem::TeleportToDestination Actor is not free for teleportation"));
						}
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("UTeleportingSubsystem::TeleportToDestination Actor is null"));
				}
			}

			if (DestinationActor && TeleportingActor)
			{
				FVector TeleportingActorLocation = TeleportingActor->GetActorLocation();
				FVector Difference = FVector::ZeroVector;
				FVector RootShiftNew;
				float Shift = -10000;
				FVector EffectScale = FVector(1.f, 1.f, 1.f);
				FRotator EffectRotation = FRotator::ZeroRotator;

				FString SlotName = FString(TEXT(""));
				USlotSceneComponent* SlotComponent = nullptr;

				FVector OriginalScale = FVector(1.f, 1.f, 1.f);

				FVector ODifference = FVector::ZeroVector;

				ATeleportDestination* TeleportDestination = Cast<ATeleportDestination>(DestinationActor);
				for (USlotSceneComponent* Slot : TeleportDestination->Slots)
				{
					if (!Slot)
					{
						continue;
					}

					if (Slot->GetActiveSlot() == false)
					{
						UE_LOG(LogTemp, Warning, TEXT("Slot %s is not active"), *Slot->GetName());
						UTeleportFailResponseObject* FailResponse = NewObject<UTeleportFailResponseObject>(this);
						FailResponse->ObjectId = ObjectId;
						FailResponse->DestinationId = DestinationId;
						FailResponse->Response = FString(TEXT("Destination ")) + DestinationId + FString(TEXT(" Slot ")) + Slot->SlotName.ToString() + FString(TEXT(" is not active"));
						TeleportFailResponses.Add(FailResponse);

						continue;
					}

					if (Slot->IsInCooldown())
					{
						UE_LOG(LogTemp, Warning, TEXT("Slot %s is in cooldown"), *Slot->GetName());
						UTeleportFailResponseObject* FailResponse = NewObject<UTeleportFailResponseObject>(this);
						FailResponse->ObjectId = ObjectId;
						FailResponse->DestinationId = DestinationId;
						FailResponse->Response = FString(TEXT("Destination ")) + DestinationId + FString(TEXT(" Slot ")) + Slot->SlotName.ToString() + FString(TEXT(" is in cooldown"));
						TeleportFailResponses.Add(FailResponse);

						continue;
					}

					FVector OriginalOrigin;
					FVector OriginalBoxExtent;
					TeleportingActor->GetActorBounds(true, OriginalOrigin, OriginalBoxExtent, true);
					OriginalScale = FVector(OriginalBoxExtent.X / 50, OriginalBoxExtent.Y / 50, OriginalBoxExtent.Z / 50);
					ODifference = OriginalOrigin - TeleportingActorLocation;

					FTransform OriginalTransform = TeleportingActor->GetActorTransform();
					TeleportingActor->SetActorRotation(FRotator::ZeroRotator);

					FVector Origin;
					FVector BoxExtent;
					TeleportingActor->GetActorBounds(true, Origin, BoxExtent, true);

					float H1 = BoxExtent.Z * 2.f;

					TeleportingActor->SetActorTransform(OriginalTransform);

					Difference = Origin - TeleportingActorLocation;

					FVector DestinationLocation = Slot->GetComponentLocation();
					FRotator DestinationRotation = Slot->GetComponentRotation();

					FVector RootShift = FVector(0, 0, 1);

					FVector OriginNew;
					FVector BoxExtentNew;
					FRotator BoxRotation;

					GetReorientedActorBounds(TeleportingActor, Slot, OriginNew, BoxExtentNew, BoxRotation);

					RootShiftNew = BoxRotation.RotateVector(RootShift);
					Difference = BoxRotation.RotateVector(Difference);
					EffectScale = FVector(BoxExtent.X / 50, BoxExtent.Y / 50, BoxExtent.Z / 50);
					EffectRotation = BoxRotation;

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
					else
					{
						UTeleportFailResponseObject* FailResponse = NewObject<UTeleportFailResponseObject>(this);
						FailResponse->ObjectId = ObjectId;
						FailResponse->DestinationId = DestinationId;
						FailResponse->Response = FString(TEXT("Destination ")) + DestinationId + FString(TEXT(" Slot ")) + Slot->SlotName.ToString() + FString(TEXT(" is blocked"));
						TeleportFailResponses.Add(FailResponse);
					}
				}

				if (SlotComponent)
				{

					if (TeleportDestination->TeleportingEffect)
					{
						//TeleportDestination->StartTeleportEffect->SetFloat

						Niagara = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
							TeleportingActor->GetWorld(),
							TeleportDestination->TeleportingEffect,
							TeleportingActor->GetActorLocation() + ODifference,
							TeleportingActor->GetActorRotation(),
							EffectScale,
							true
						);

						if (Niagara)
						{
							Niagara->SetVariableFloat(TEXT("Duration"), TeleportDestination->TeleportationDuration);
						}
					}

					if (TeleportDestination->TeleportingEffect)
					{
						Niagara = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
							TeleportingActor->GetWorld(),
							TeleportDestination->TeleportingEffect,
							SlotComponent->GetComponentLocation() + RootShiftNew * Shift,// - Difference,
							EffectRotation,
							EffectScale,
							true
						);

						if (Niagara)
						{
							Niagara->SetVariableFloat(TEXT("Duration"), TeleportDestination->TeleportationDuration);
						}
					}

					ActorTeleport = TeleportingActor;
					ActorDestination = TeleportDestination;
					TeleportLocation = SlotComponent->GetComponentLocation() + RootShiftNew * Shift - Difference;
					TeleportRotation = SlotComponent->GetComponentRotation();

					if (TeleportDestination->TeleportationDuration > 0)
					{
						TeleportDestination->StartTeleportation();
						FTimerHandle TimerHandle;

						GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UTeleportingSubsystem::Teleporting, TeleportDestination->TeleportationDuration / 2, false);
					}
					/*
					if (TeleportDestination->TeleportationDuration > 0)
					{
						FTimerHandle TimerHandle;
						GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
							{
								if(Niagara) Niagara->DestroyComponent();
							}, TeleportDestination->TeleportationDuration, false);
					}
					*/
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
					UTeleportFailResponseObject* FailResponse = NewObject<UTeleportFailResponseObject>(this);
					FailResponse->ObjectId = ObjectId;
					FailResponse->DestinationId = DestinationId;
					FailResponse->Response = FString(TEXT("Destination ")) + DestinationId + FString(TEXT(" No suitable slot found for teleportation"));
					TeleportFailResponses.Add(FailResponse);

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

	UE_LOG(LogTemp, Warning, TEXT("No teleport row found for Object ID: %s and Destination ID: %s"), *ObjectId, *DestinationId);
}

void UTeleportingSubsystem::Teleporting()
{
	ActorTeleport->SetActorLocationAndRotation(TeleportLocation, TeleportRotation);

	ActorDestination->FinishTeleportation();
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

	TArray<FVector> LocalCorners;
	LocalCorners.Add(FVector(BoxExtent.X, BoxExtent.Y, BoxExtent.Z));
	LocalCorners.Add(FVector(BoxExtent.X, BoxExtent.Y, -BoxExtent.Z));
	LocalCorners.Add(FVector(BoxExtent.X, -BoxExtent.Y, BoxExtent.Z));
	LocalCorners.Add(FVector(BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z));
	LocalCorners.Add(FVector(-BoxExtent.X, BoxExtent.Y, BoxExtent.Z));
	LocalCorners.Add(FVector(-BoxExtent.X, BoxExtent.Y, -BoxExtent.Z));
	LocalCorners.Add(FVector(-BoxExtent.X, -BoxExtent.Y, BoxExtent.Z));
	LocalCorners.Add(FVector(-BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z));

	FQuat SlotRotation = Slot->GetComponentQuat();
	TArray<FVector> RotatedCorners;
	for (const FVector& Local : LocalCorners)
	{
		RotatedCorners.Add(SlotRotation.RotateVector(Local));
	}

	FBox ReorientedBox(EForceInit::ForceInit);
	for (const FVector& Corner : RotatedCorners)
	{
		ReorientedBox += Corner;
	}

	OutOrigin = ReorientedBox.GetCenter() + Origin;
	OutExtent = ReorientedBox.GetExtent();
	OutRotation = SlotRotation.Rotator();
}
