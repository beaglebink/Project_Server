#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SaveSubsystems/I_SaveableObject.h"
#include "FlyerPosting/O_FlyerPostingStructures.h"
#include "A_FlyerPostingBase.generated.h"

class UInteractiveItemComponent;
class UFloorAssignmentComponent;
class UCanvasRenderTarget2D;

UCLASS()
class ALSEXTRAS_API AA_FlyerPostingBase : public AActor, public II_SaveableObject
{
	GENERATED_BODY()

public:
	AA_FlyerPostingBase();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Destroyed() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FLyerPosting")
	FName ChoreId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FLyerPosting")
	uint8 bIsAutoAcivated : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLyerPosting")
	uint8 bIsDeferredEvaluate : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLyerPosting")
	FGameplayTag CompletedTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLyerPosting")
	FGameplayTag FailedTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLyerPosting", meta = (ClampMin = 0))
	int32 AvailableFlyersQuantityForPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLyerPosting", meta = (ClampMin = 0))
	int32 RequiredFlyersQuantityToPlace;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FLyerPosting")
	FGameplayTag FlyerTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UInteractiveItemComponent* InteractiveItemComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UFloorAssignmentComponent* FloorAssignmentComponent;

private:
	UPROPERTY()
	EChoreStatus ChoreStatus;

public:
	UFUNCTION(BlueprintCallable, Category = "ChoreStatus")
	void SetChoreStatus(EChoreStatus NewStatus);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ChoreStatus")
	EChoreStatus GetChoreStatus()const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FLyerPosting")
	FGameplayTag PublicResultState(bool Result);

	// II_SaveableObject interface
	virtual FString GetFloorAssignmentComponentIdAsString_Implementation() override;

	virtual int32 CollectFlyerPostingSaveData_Implementation() override;

	virtual void ApplyFlyerPostingSaveData_Implementation(const int32 SaveChoreData) override;

	// Intermediate data saving
	UFUNCTION(BlueprintCallable, Category = "Saving")
	FString CollectIntermediateSaveData();

	UFUNCTION(BlueprintCallable, Category = "Saving")
	void ApplyIntermediateSaveData(FString Jsonstring);

private:
	bool CheckIfStarted();

	UPROPERTY()
	UCanvasRenderTarget2D* TextRenderTarget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FlyerPosting", meta = (AllowPrivateAccess = true))
	UStaticMesh* PlacementStaticMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FlyerPosting", meta = (AllowPrivateAccess = true))
	UMaterialInterface* FlyerMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* MID_Flyer;

	UFUNCTION()
	void DrawZoneText(UCanvas* Canvas, int32 Width, int32 Height);

public:
	UFUNCTION(CallInEditor, Category = "FlyerPlacement")
	void AddPlacementBox();

	UFUNCTION(BlueprintImplementableEvent, Category = "Chore")
	void OnStarted();
};
