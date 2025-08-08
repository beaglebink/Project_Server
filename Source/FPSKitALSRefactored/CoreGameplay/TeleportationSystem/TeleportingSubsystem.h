#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/DataTable.h"
#include "TeleportingSubsystem.generated.h"

/**
 * Строка таблицы телепортации.
 */
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FTeleportTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText ObjectID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DestinationID;
};

/**
 * Подсистема телепортации, получающая доступ к таблице сцен через интерфейс GameInstance.
 */
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

private:
    UPROPERTY()
    TArray<AActor*> TeleportingDestinations;

    UDataTable* LoadedSceneTable;

    /*


    UFUNCTION(BlueprintCallable, Category = "Teleporting")
    FTeleportTableRow GetTeleportRowByID(FName RowID) const;

protected:

    UPROPERTY()
    UDataTable* LoadedTeleportTable;


    */
};