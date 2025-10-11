#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_OrderSystem.generated.h"

class AA_OrderCube;

UCLASS()
class ALSEXTRAS_API AA_OrderSystem : public AActor
{
	GENERATED_BODY()

public:
	AA_OrderSystem();

protected:
	virtual void OnConstruction(const FTransform& Transform)override;

	virtual void BeginPlay() override;

	virtual void Destroyed()override;

public:
	virtual void Tick(float DeltaTime) override;

	void CheckHitCubeRightOrderIndex(AA_OrderCube* OrderCube);

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	TSubclassOf<AA_OrderCube> OrderCubeClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components|Property", meta = (AllowPrivateAccess = true, ClampMin = "0", ClampMax = "10"))
	int32 CubesQuantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components|Property", meta = (AllowPrivateAccess = true, ClampMin = "0", ClampMax = "10"))
	int32 Damage = 0;

	int32 PrevIndex = 0;

	UPROPERTY()
	TArray<AA_OrderCube*> CubesArray;

	void DestroyCubes();
};
