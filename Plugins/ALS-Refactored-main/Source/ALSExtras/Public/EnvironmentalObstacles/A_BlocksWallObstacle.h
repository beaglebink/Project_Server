#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_BlocksWallObstacle.generated.h"

class AA_BlockObstacle;
class UTextRenderComponent;
enum class EDirection :uint8;

UCLASS()
class ALSEXTRAS_API AA_BlocksWallObstacle : public AActor
{
	GENERATED_BODY()

public:
	AA_BlocksWallObstacle();

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)override;
#endif

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void Destroyed() override;
#endif

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	TSubclassOf<AA_BlockObstacle> BlockObstacleClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	FIntPoint WallDimensions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true, ClampMin = "2", ClampMax = "100"))
	int32 CriticalBlocksTreshold;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	uint8 bUseTimeInterval : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true, ClampMin = "10", EditCondition = "bUseTimeInterval"))
	float TimeIntervalBetweenSwaps = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	uint8 bUseNumberOfBlocksDestroyed : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true, EditCondition = "bUseNumberOfBlocksDestroyed"))
	int32 NumberOfBlocksDestroyedThreshold = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	uint8 bUsePlayerAccuracy : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true, EditCondition = "bUsePlayerAccuracy"))
	int32 PlayerAccuracyThreshold = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	int32 TotalBlocksDestroyedTillDestroyWall = 1;

	UPROPERTY()
	TArray<AA_BlockObstacle*> WallBlocks;

	UPROPERTY()
	FVector BlockExtent;

	void PrepareBlockSwaps();

	void ProcessBlockSwaps();

	int32 GetNeighborIndex(int32 Index, EDirection Direction) const;

	bool IsOppositeDirection(EDirection A, EDirection B) const;

	EDirection GetOppositeDirection(EDirection Direction) const;

	TArray<UTextRenderComponent*> DebugGridTexts;

public:
	uint8 bIsProcessingSwaps : 1{ false};

	uint8 bIsOnDestroy : 1{ false};

	void NotifyBlockDestroyed();

	void NotifyPlayerShot(bool bIsAccurate);

	void CheckAndHandleCompletedSwaps();

	void UpperBlocksFall(int32 Index);

private:
	int32 BlocksOnSwapCount = 0;

	int32 BlocksDestroyedSinceLastSwap = 0;

	int32 PlayerShotsSinceLastSwap = 0;

	void DestroyWall();

	//debug function
	void DrawGrid();
};
