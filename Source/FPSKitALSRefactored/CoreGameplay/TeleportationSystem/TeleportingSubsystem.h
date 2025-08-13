#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "TeleportingSubsystem.generated.h"


USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FTeleportTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString ObjectID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DestinationID;
};


UCLASS()
class FPSKITALSREFACTORED_API UTeleportingSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    
    UFUNCTION(BlueprintCallable, Category = "Teleportation")
    void RegistrationTeleportingDestination(AActor* Destination);

    UFUNCTION(BlueprintCallable, Category = "Teleportation")
    void UnregistrationTeleportingDestination(AActor* Destination);

    UFUNCTION(BlueprintCallable, Category = "Teleportation")
	void RegistrationTeleportingActor(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "Teleportation")
	void UnregistrationTeleportingActor(AActor* Actor);

    UFUNCTION(BlueprintCallable, Category = "Teleportation")
	void TeleportToDestination(FString ObjectId, FString DestinationId);

private:
    void GetReorientedActorBounds(const AActor* Actor, const USceneComponent* Slot, FVector& OutOrigin, FVector& OutExtent, FRotator& OutRotation);

private:
    UPROPERTY()
    TArray<AActor*> TeleportingDestinations;

    UPROPERTY()
    TArray<AActor*> TeleportingActors;

    UDataTable* LoadedSceneTable;
};