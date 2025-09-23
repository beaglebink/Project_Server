#pragma once

#include "Components/TimelineComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_ChessBoard.generated.h"

class UBoxComponent;
class AAlsCharacterExample;
class UPhysicsConstraintComponent;

USTRUCT(BlueprintType)
struct FCell
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UStaticMeshComponent* MeshComponent;

	UPROPERTY()
	UPhysicsConstraintComponent* ConstraintComponent;

	UPROPERTY()
	UBoxComponent* TriggerComponent;

	UPROPERTY()
	UAudioComponent* AudioComponent;

	UPROPERTY()
	UMaterialInstanceDynamic* CellDynamicMaterial;
};

UCLASS()
class ALSEXTRAS_API AA_ChessBoard : public AActor
{
	GENERATED_BODY()

public:
	AA_ChessBoard();

private:
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "10"))
	int32 Rows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "10"))
	int32 Columns;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "50"))
	float CellDamage;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field|CellRotation", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "60"))
	float MinTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field|CellRotation", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "60"))
	float MaxTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field", meta = (AllowPrivateAccess = "true"))
	UStaticMesh* CellMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field", meta = (AllowPrivateAccess = "true"))
	USoundBase* RotateSound;

	UPROPERTY()
	TArray<FCell> ChessField;

	TMap<AAlsCharacterExample*, TSet<FCell*>> CharactersToCells;

	TArray<int32> CellsToRotate;

	TMap<int32, FRotator> InitialRotations;

	FQuat TargetRotation;

	void BuildField();

	UFUNCTION()
	void OnCellBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnCellEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void DoCharacterDamage();

	void ScheduleNextTimer();

	void RotateCells();

	FQuat CountTargetRotationInDependsOnPrevRotation(const FRotator& CurrentRot);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* RotateTimeline;

	UPROPERTY(EditDefaultsOnly, Category = "Timeline")
	UCurveFloat* RotateFloatCurve;

	FOnTimelineFloat RotateProgressFunction;

	FOnTimelineEvent RotateFinishedFunction;

	UFUNCTION()
	void RotateTimelineProgress(float Value);

	UFUNCTION()
	void RotateTimelineFinished();

	void UpdateCellMaterial(int32 Index, float Value);
};
