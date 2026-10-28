#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SaveSubsystems/I_SaveableObject.h"
#include "A_ChorePlacementZone.generated.h"

class AA_DropZone;
class AA_AreaDropZone;
class AA_DropZoneGridVisual;
class AA_InteractableActor;
class UFloorAssignmentComponent;

UENUM(BlueprintType)
enum class EWrongItemBehavior : uint8
{
	Ignore,
	CountAsMistake,
	BlockCompletion
};

USTRUCT(BlueprintType)
struct FDropZonePlacementZone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement|DropZones")
	TSubclassOf<AA_InteractableActor> ItemClass;

	FVector ItemMeshSize{ 0.0f,0.0f,0.0f };

	int32 ItemsRows = 0;

	int32 ItemsColumns = 0;

	int32 DropZoneQuantity = 0;

	UPROPERTY()
	AA_DropZoneGridVisual* DropZoneGridVisual;
};

USTRUCT(BlueprintType)
struct FSourceItemData
{
	GENERATED_BODY()

	UPROPERTY()
	AA_InteractableActor* ItemActor;

	UPROPERTY()
	FTransform ItemDefaultTransform;
};

USTRUCT(BlueprintType)
struct FChoreResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chore")
	uint8 bResult : 1{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chore")
	TMap<FGameplayTag, int32> SuccessedItems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chore")
	TMap<FGameplayTag, int32> MissedItems;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chore")
	TMap<FGameplayTag, int32> FailedItems;
};

UCLASS()
class FPSKITALSREFACTORED_API AA_ChorePlacementZone : public AActor, public II_SaveableObject
{
	GENERATED_BODY()

public:
	AA_ChorePlacementZone();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Destroyed() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement|ParentActor")
	AActor* ParentActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChorePlacement")
	FName ChoreId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChorePlacement")
	uint8 bIsAutoAcivated : 1{true};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement")
	uint8 bIsDeferredEvaluate : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement|Wrong Items Behavior")
	EWrongItemBehavior ItemBehavior;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement")
	FGameplayTag CompletedTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement")
	FGameplayTag FailedTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement|Breakable")
	int32 MaxBrokenItemsAllowed = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ChorePlacement|SourceZone")
	UStaticMeshComponent* SourceZoneMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ChorePlacement|SourceZone")
	UMaterialInterface* SourceZoneMeshMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* DMI_SourceZoneMeshMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement|DropZones")
	TSubclassOf<AA_AreaDropZone> AreaDropZoneClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement|DropZones")
	TSubclassOf<AA_DropZone> DropZoneClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement|DropZones")
	TSubclassOf<AA_DropZoneGridVisual> DropZoneGridVisualClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement|DropZones", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float GapSizeBetweenInPercent = 0.1f;

	UPROPERTY(BlueprintReadOnly, Category = "ChorePlacement|DropZones")
	AA_AreaDropZone* AreaDropZone;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement|DropZones")
	TArray<FDropZonePlacementZone> DropZonePlacementZones;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UFloorAssignmentComponent* FloorAssignmentComponent;

	UFUNCTION(CallInEditor, Category = "ChorePlacement")
	void UpdateSourcePlacedItems();

	UPROPERTY()
	TMap<FGuid, FSourceItemData> SourcePlacedItems;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "ChorePlacement|DropZones")
	TArray<FGameplayTag> SourcePlacedItemTags;

	UFUNCTION()
	void CountBreaks(AA_Breakable* BreakableActor);

private:
	UPROPERTY()
	FTimerHandle AutoResultTimerHandle;

	int32 BrokenItemsCount = 0;

	UPROPERTY()
	EChoreStatus ChoreStatus;

public:
	UFUNCTION(BlueprintCallable, Category = "ChorePlacement")
	void SetChoreStatus(EChoreStatus NewStatus);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ChorePlacement")
	EChoreStatus GetChoreStatus()const;

	UFUNCTION(BlueprintCallable, Category = "ChorePlacement")
	void ResetChorePlacementZone();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ChorePlacement")
	FChoreResult GetResultState();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ChorePlacement")
	FGameplayTag PublicResultState(FChoreResult ChoreResult);

	// II_SaveableObject interface
	virtual FSaveChoreData CollectChorePlacementSaveData_Implementation() override;

	virtual void ApplyChorePlacementSaveData_Implementation(const FSaveChoreData& SaveChoreData) override;

	// intermediate data saving
	UFUNCTION(BlueprintCallable, Category = "Saving")
	FString CollectIntermediateSaveData();

	UFUNCTION(BlueprintCallable, Category = "Saving")
	void ApplyIntermediateSaveData(FString Jsonstring);
};
