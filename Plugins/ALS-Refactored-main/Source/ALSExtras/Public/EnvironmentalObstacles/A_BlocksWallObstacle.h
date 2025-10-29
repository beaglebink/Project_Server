#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_BlocksWallObstacle.generated.h"

UCLASS()
class ALSEXTRAS_API AA_BlocksWallObstacle : public AActor
{
	GENERATED_BODY()
	
public:	
	AA_BlocksWallObstacle();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
