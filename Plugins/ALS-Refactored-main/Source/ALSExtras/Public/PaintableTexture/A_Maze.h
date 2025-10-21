#pragma once

#include "CoreMinimal.h"
#include "PaintableTexture/A_PaintableTexture.h"
#include "A_Maze.generated.h"

UCLASS()
class ALSEXTRAS_API AA_Maze : public AA_PaintableTexture
{
	GENERATED_BODY()

public:
	AA_Maze();

protected:
	virtual void OnConstruction(const FTransform& Transform)override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:

	void HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit);

	void PaintCell(int32 CellX, int32 CellY);
};
