#pragma once

#include "CoreMinimal.h"
#include "PaintableTexture/A_PaintableTexture.h"
#include "A_BlackTile.generated.h"

UCLASS()
class ALSEXTRAS_API AA_BlackTile : public AA_PaintableTexture
{
	GENERATED_BODY()

public:
	AA_BlackTile();

protected:
	virtual void OnConstruction(const FTransform& Transform)override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 100))
	int32 PaintedPercentThreshold = 60;

	int32 PaintedPixelCount = 0;

	uint8 bIsFirstPixel : 1{true};
	
	uint8 bShouldPaintEdgesFirst : 1{true};

	void HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit);

	void PaintCell(int32 CellX, int32 CellY);

	bool HasColoredOutline();
};
