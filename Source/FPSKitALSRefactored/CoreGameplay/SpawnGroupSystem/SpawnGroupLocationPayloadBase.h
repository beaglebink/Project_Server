// SpawnGroupLocationPayloadBase.h
#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "SpawnGroupLocationPayloadBase.generated.h"

class UWorldMapAsset;
class UWorldRegionAsset;
class UStreetAsset;
class UInteriorSetAsset;
class UFloorAsset;
class ASpawnGroupSpawner;

UCLASS(Abstract, BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupLocationPayloadBase : public UOutcomePayload
{
    GENERATED_BODY()

public:
    // Главное runtime-поле.
    UPROPERTY(BlueprintReadWrite, Category = "Target")
    FGuid GroupId;

#if WITH_EDITORONLY_DATA
    UPROPERTY(EditAnywhere, Category = "Target Location", meta = (GetOptions = "GetRegionOptions"))
    FString TargetRegion;

    UPROPERTY(EditAnywhere, Category = "Target Location", meta = (GetOptions = "GetDistrictOptions"))
    FString TargetDistrict;

    UPROPERTY(EditAnywhere, Category = "Target Location", meta = (GetOptions = "GetStreetOptions"))
    FString TargetStreet;

    UPROPERTY(EditAnywhere, Category = "Target Location", meta = (GetOptions = "GetBuildingOptions"))
    FString TargetBuilding;

    UPROPERTY(EditAnywhere, Category = "Target Location", meta = (GetOptions = "GetFloorOptions"))
    FString TargetFloor;

    UPROPERTY(EditAnywhere, Category = "Target Location", meta = (GetOptions = "GetSpawnerOptions"))
    FString TargetSpawner;
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
    UFUNCTION() TArray<FString> GetRegionOptions() const;
    UFUNCTION() TArray<FString> GetDistrictOptions() const;
    UFUNCTION() TArray<FString> GetStreetOptions() const;
    UFUNCTION() TArray<FString> GetBuildingOptions() const;
    UFUNCTION() TArray<FString> GetFloorOptions() const;
    UFUNCTION() TArray<FString> GetSpawnerOptions() const;

    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

private:
    UWorldMapAsset* FindRegion() const;
    UWorldRegionAsset* FindDistrict(UWorldMapAsset* Region) const;
    UStreetAsset* FindStreet(UWorldRegionAsset* District) const;
    UInteriorSetAsset* FindBuilding(UStreetAsset* Street) const;
    UFloorAsset* FindFloor(UInteriorSetAsset* Building) const;
    ASpawnGroupSpawner* FindSpawner(UFloorAsset* Floor) const;
#endif
};