#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_BlocksWallObstacle.generated.h"

class AA_BlockObstacle;
class UTextRenderComponent;

UCLASS()
class ALSEXTRAS_API AA_BlocksWallObstacle : public AActor
{
	GENERATED_BODY()

public:
	AA_BlocksWallObstacle();

protected:
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	TSubclassOf<AA_BlockObstacle> BlockObstacleClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	uint8 bUseTimeInterval : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	uint8 bUseNumberOfBlocksDestroyed : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	uint8 bUsePlayerAccuracy : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	FIntPoint WallDimensions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true, ClampMin = "0", ClampMax = "100"))
	int32 CriticalBlocksTreshold;

	UPROPERTY()
	TArray<AA_BlockObstacle*> WallBlocks;

	UPROPERTY()
	FVector BlockExtent;

	TArray<UTextRenderComponent*> DebugGridTexts;

public:
	void UpperBlocksFall(int32 Index);

	//debug function
	void DrawGrid();
};
