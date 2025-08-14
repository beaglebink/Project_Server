#pragma once

#include "CoreMinimal.h"
#include "TeleportFailResponseObject.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "TeleportingSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTeleportation, ATeleportDestination*, Destination, USlotSceneComponent*, Slot, AActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorStartCooldown, ATeleportDestination*, Destination);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorFinishCooldownSub, ATeleportDestination*, Destination);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStartSlotCooldown, ATeleportDestination*, Destination, USlotSceneComponent*, Slot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStopSlotCooldownSub, ATeleportDestination*, Destination, USlotSceneComponent*, Slot);

/*
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API UTeleportFailResponseObject : public UObject
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString ObjectId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString DestinationId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString Response;
};
*/
//DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTeleportationFailed, FString, ActorId, FString, DestinationId, TArray< FTeleportFailResponse>, TeleportationFailResponses);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnTeleportationFailed,
    FString, ObjectId,
    FString, DestinationId
);


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

    UFUNCTION()
    void DestinationFinishCooldown(ATeleportDestination* Destination);

    UFUNCTION()
    void SlotFinishCooldown(ATeleportDestination* Destination, USlotSceneComponent* Slot);

    UFUNCTION(BlueprintCallable, Category = "Teleportation")
	TArray<UTeleportFailResponseObject*> GetTeleportFailResponses() const { return TeleportFailResponses; }

public:
    UPROPERTY(BlueprintAssignable, Category = "Teleportation")
	FOnTeleportation OnTeleportation;

    UPROPERTY(BlueprintAssignable, Category = "Teleportation")
    //FOnTeleportationFailed OnTeleportationFailed;
    FOnTeleportationFailed OnTeleportationFailed;

    UPROPERTY(BlueprintAssignable, Category = "Teleportation")
    FOnActorStartCooldown OnDestinationStartCooldown;

    UPROPERTY(BlueprintAssignable, Category = "Teleportation")
    FOnActorFinishCooldownSub OnDestinationFinishCooldown;

    UPROPERTY(BlueprintAssignable, Category = "Slot")
    FOnStartSlotCooldown OnSlotStartCooldown;

    UPROPERTY(BlueprintAssignable, Category = "Slot")
    FOnStopSlotCooldownSub OnSlotFinishCooldown;

private:
    void GetReorientedActorBounds(const AActor* Actor, const USceneComponent* Slot, FVector& OutOrigin, FVector& OutExtent, FRotator& OutRotation);

protected:
    UPROPERTY(EditAnywhere)
    TArray< UTeleportFailResponseObject*> TeleportFailResponses;
private:
    UPROPERTY()
    TArray<AActor*> TeleportingDestinations;

    UPROPERTY()
    TArray<AActor*> TeleportingActors;

    UDataTable* LoadedSceneTable;


};