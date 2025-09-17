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

private:
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties", meta = (AllowPrivateAccess = "true", EditCondition = "AdType == EnumAdType::Drifter", EditConditionHides, ToolTip = "Minimum speed AdWall can move", ClampMin = "0", ClampMax = "300"))
	float MinSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties", meta = (AllowPrivateAccess = "true", EditCondition = "AdType == EnumAdType::Drifter", EditConditionHides, ToolTip = "Maximum speed AdWall can move", ClampMin = "0", ClampMax = "300"))
	float MaxSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties", meta = (AllowPrivateAccess = "true", EditCondition = "AdType == EnumAdType::Drifter", EditConditionHides, ToolTip = "Minimum time before AdWall changes velocity", ClampMin = "0", ClampMax = "10"))
	float MinTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties", meta = (AllowPrivateAccess = "true", EditCondition = "AdType == EnumAdType::Drifter", EditConditionHides, ToolTip = "Maximum time before AdWall changes velocity", ClampMin = "0", ClampMax = "10"))
	float MaxTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties", meta = (AllowPrivateAccess = "true", EditCondition = "AdType == EnumAdType::Malicious", EditConditionHides, ClampMin = "0", ClampMax = "20"))
	float AdDamage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Properties", meta = (AllowPrivateAccess = "true"))
	uint8 bShouldDoKnockback : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material", meta = (AllowPrivateAccess = "true"))
	UTexture2D* AdTexture;

	FVector TargetVelocity;

	uint8 bIsHitAdWall : 1{false};

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

	bool AdWallsMoreThan_25();

	void SpawnAd();

	void DriftAd();

	void ScheduleNextTimer();

	FVector SetTargetVelocity();

	UFUNCTION()
	void OnAdWallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnCrossHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

public:
	UFUNCTION(BlueprintCallable, Category = "Material")
	void UpdateScreenMaterial();
};
