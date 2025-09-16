#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_AdWall.generated.h"

class UProjectileMovementComponent;

UENUM()
enum class EnumAdType :uint8
{
	Standard	UMETA(DisplayName = "Standard"),
	Drifter		UMETA(DisplayName = "Drifter"),
	Inflator	UMETA(DisplayName = "Inflator"),
	Malicious	UMETA(DisplayName = "Malicious")
};

UCLASS()
class ALSEXTRAS_API AA_AdWall : public AActor
{
	GENERATED_BODY()

public:
	AA_AdWall();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* AdWallComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* CrossComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* MovementComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties", meta = (AllowPrivateAccess = "true"))
	EnumAdType AdType;

	float AdDamage = 0.0f;

	FVector TargetVelocity;

	bool AdWallsMoreThan_25();

	void SpawnAd();
	
	void DriftAd();

	void ScheduleNextTimer();
};
