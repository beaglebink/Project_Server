#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_WireObstacle.generated.h"

class UBoxComponent;

UCLASS()
class ALSEXTRAS_API AA_WireObstacle : public AActor
{
	GENERATED_BODY()

public:
	AA_WireObstacle();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UBoxComponent* BoxComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters", meta = (AllowPrivateAccess = true, ClampMin = "0"))
	float WireObstacleRadius = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters", meta = (AllowPrivateAccess = true, ClampMin = "0"))
	int32 AnchorsPerSide = 50;

	UPROPERTY(VisibleAnywhere, Category = "Wires|Debug")
	TArray<FVector> LeftAnchors;

	UPROPERTY(VisibleAnywhere, Category = "Wires|Debug")
	TArray<FVector> RightAnchors;

	UPROPERTY(VisibleAnywhere, Category = "Wires|Debug")
	TArray<FVector> LeftHitPoints;

	UPROPERTY(VisibleAnywhere, Category = "Wires|Debug")
	TArray<FVector> RightHitPoints;

	template<typename T>
	void ShuffleArray(TArray<T>& Array);
};
