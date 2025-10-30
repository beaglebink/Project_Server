#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/I_WeaponInteraction.h"
#include "Components/TimelineComponent.h"
#include "A_BlocksWallObstacle.generated.h"

UENUM(BlueprintType)
enum class EBlockState :uint8
{
	Neutral,
	Critical,
	Destroyed,
	Moving,
	Falling,
	Shot
};

USTRUCT(BlueprintType)
struct FWallBlock
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlockParameters")
	EBlockState BlockState = EBlockState::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlockParameters")
	TObjectPtr<UStaticMeshComponent> BlockMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlockParameters")
	FVector InitialLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlockParameters")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlockParameters")
	TObjectPtr<UMaterialInstanceDynamic> BlockMaterialInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlockParameters")
	FIntPoint GridPosition;
};

USTRUCT(BlueprintType)
struct FBlockRow
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FWallBlock> Row;
};

class UNiagaraSystem;
class UAudioComponent;

UCLASS()
class ALSEXTRAS_API AA_BlocksWallObstacle : public AActor, public II_WeaponInteraction
{
	GENERATED_BODY()

public:
	AA_BlocksWallObstacle();

protected:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	uint8 bUseTimeInterval : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	uint8 bUseNumberOfBlocksDestroyed : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	uint8 bUsePlayerAccuracy : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	UStaticMesh* BlockMeshAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	UMaterialInterface* BlockMaterialAsset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	UNiagaraSystem* BlockDestroyFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	UAudioComponent* AudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	USoundBase* BlockDestroySound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	USoundBase* BlockMoveSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	USoundBase* BlockFallSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	FIntPoint WallDimensions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true, ClampMin = "0", ClampMax = "100"))
	int32 CriticalBlocksTreshold;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	TArray<FBlockRow> WallBlocks;
	FWallBlock* CurrentBlock = nullptr;
	uint8 bBlockCoolDown : 1 {false};

	void HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit);

	void SetBlockState(FIntPoint GridPosition, EBlockState NewState);

	void SetBlockMaterial(FIntPoint GridPosition, bool bIsCritical);

	void OnShotMaterial(FIntPoint GridPosition, bool bIsShot);

	void StartBlockDestroy(FIntPoint GridPosition);

	void MoveBlock(FIntPoint GridPosition);

	void StartBlockFall(FIntPoint GridPosition);

protected:
	//Destroy Timeline
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* DestroyTimeline;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* DestroyFloatCurve;

	FOnTimelineFloat DestroyProgressFunction;

	FOnTimelineEvent DestroyFinishedFunction;

	UFUNCTION()
	void DestroyTimelineProgress(float Value);

	UFUNCTION()
	void DestroyTimelineFinished();

	//Move Timeline
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* MoveBlockTimeline;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* MoveBlockFloatCurve;

	FOnTimelineFloat MoveBlockProgressFunction;

	FOnTimelineEvent MoveBlockFinishedFunction;

	UFUNCTION()
	void MoveBlockTimelineProgress(float Value);

	UFUNCTION()
	void MoveBlockTimelineFinished();
};
