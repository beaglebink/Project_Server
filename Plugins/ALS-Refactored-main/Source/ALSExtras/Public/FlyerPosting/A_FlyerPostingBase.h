#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "FlyerPosting/O_FlyerPostingStructures.h"
#include "A_FlyerPostingBase.generated.h"

class UFloorAssignmentComponent;
class UInteractiveItemComponent;
class UCanvasRenderTarget2D;
class AA_Flyer;

UCLASS()
class ALSEXTRAS_API AA_FlyerPostingBase : public AActor
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FlyerPosting")
	FName ChoreId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FlyerPosting")
	uint8 bIsDeferredEvaluate : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FlyerPosting")
	FGameplayTag CompletedTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FlyerPosting")
	FGameplayTag FailedTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FlyerPosting", meta = (ClampMin = 0))
	int32 AvailableFlyersQuantityForPlayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FlyerPosting", meta = (ClampMin = 0))
	int32 RequiredFlyersQuantityToPlace;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "FlyerPosting")
	FGameplayTag FlyerTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FlyerPosting")
	TSubclassOf<AA_Flyer> FlyerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UFloorAssignmentComponent* FloorAssignmentComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UInteractiveItemComponent* InteractiveItemComponent;

private:
	UPROPERTY()
	EChoreStatus ChoreStatus;

	UPROPERTY()
	uint8 bWasStarted : 1{false};

	UPROPERTY()
	TMap<FGuid, FTransform> PlacedFlyers;

public:
	UFUNCTION(BlueprintCallable, Category = "ChoreStatus")
	void SetChoreStatus(EChoreStatus NewStatus);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ChoreStatus")
	EChoreStatus GetChoreStatus()const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FlyerPosting")
	FGameplayTag PublicResultState(bool Result);

	// Data saving
	UFUNCTION(BlueprintCallable, Category = "Saving")
	FString CollectSaveData();

	UFUNCTION(BlueprintCallable, Category = "Saving")
	void ApplySaveData(FString Jsonstring);

private:
	FTimerHandle TooltipCheckHandle;

	UFUNCTION()
	void OnInteract(UInteractivePickerComponent* Picker);

	UFUNCTION()
	void OnInteractiveReceiveFocus(AActor* WhoInteract);

	UFUNCTION()
	void OnInteractiveLostFocus(ACharacter* User);

	UPROPERTY()
	UCanvasRenderTarget2D* TextRenderTarget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FlyerPosting", meta = (AllowPrivateAccess = true))
	UStaticMesh* PlacementStaticMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FlyerPosting", meta = (AllowPrivateAccess = true))
	UMaterialInterface* FlyerZoneMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* MID_FlyerZone;

	UFUNCTION()
	void DrawZoneText(UCanvas* Canvas, int32 Width, int32 Height);

	FGuid CreateGuidForFlyer();

	void SpawnFlyer(FGuid FlyerId, FTransform FlyerTransform);

	FHitResult TraceForFlyer();

	FTransform GetFlyerTransform() const;

	void FlyersSpawnObserver(bool IsSpawned);

public:
	UFUNCTION(CallInEditor, Category = "FlyerPlacement")
	void AddPlacementBox();

	UFUNCTION(BlueprintImplementableEvent, Category = "Chore")
	void OnStarted();
};
