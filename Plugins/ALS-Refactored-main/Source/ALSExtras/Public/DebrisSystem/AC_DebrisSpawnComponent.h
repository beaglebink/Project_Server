#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DebrisSystem/A_Debris.h"
#include "AC_DebrisSpawnComponent.generated.h"

class AA_Debris;

UENUM(BlueprintType)
enum class EDebrisSpawnShape : uint8
{
	Sphere,
	Capsule,
	Disc,
	Plane,
	Box
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSEXTRAS_API UAC_DebrisSpawnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAC_DebrisSpawnComponent();

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)override;
#endif

	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true))
	TSubclassOf<AA_Debris> DebrisClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true))
	TArray<UStaticMesh*> DebrisMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true))
	EDebrisSpawnShape SpawnShape;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true,
		EditCondition = "SpawnShape == EDebrisSpawnShape::Sphere || SpawnShape == EDebrisSpawnShape::Capsule || SpawnShape == EDebrisSpawnShape::Disc", EditConditionHides))
	float SphereRadius = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, EditCondition = "SpawnShape == EDebrisSpawnShape::Capsule", EditConditionHides))
	float CapsuleHalfHeight = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, EditCondition = "SpawnShape == EDebrisSpawnShape::Box", EditConditionHides))
	FVector BoxExtent = FVector(100.0f, 100.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, EditCondition = "SpawnShape == EDebrisSpawnShape::Plane", EditConditionHides))
	FVector2D PlaneSize = FVector2D(100.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true))
	uint8 bShouldMeshSimulatePhysics : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, ClampMin = 0))
	int32 DebrisCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, ClampMin = 0.0f, ClampMax = 1.0f))
	float SpawnDensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, ClampMin = 0.0f))
	float ReplenishTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, ClampMin = 0.01f))
	float SuctionDifficultyMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, ClampMin = 0.0f, ClampMax = 1.0f))
	float Slipperiness = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, ClampMin = 0.0f))
	float SlipperinessImpulsePower = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, ClampMin = 0.0f))
	float AttachmentPower = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, ClampMin = 0.0f, ClampMax = 100.0f))
	float GarbagePercentage = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true))
	EDebrisFloatPattern DebrisBehavior = EDebrisFloatPattern::Stationary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, EditCondition = "DebrisBehavior != EDebrisFloatPattern::Stationary", EditConditionHides))
	float DebrisMovementSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, EditCondition = "DebrisBehavior == EDebrisFloatPattern::Erratic", EditConditionHides))
	float ErraticRadius = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true))
	EDebrisReactToPlayer PlayerReaction = EDebrisReactToPlayer::Passive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, EditCondition = "PlayerReaction != EDebrisReactToPlayer::Passive", EditConditionHides))
	float PlayerInfluenceStrength;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, EditCondition = "PlayerReaction != EDebrisReactToPlayer::Passive", EditConditionHides))
	float PlayerInfluenceRadius;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true))
	EBuoyancyType BuoyancyType = EBuoyancyType::Fixed;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, EditCondition = "BuoyancyType != EBuoyancyType::Fixed", EditConditionHides, ClampMin = 0.0f))
	float BuoyancySpeed = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true, EditCondition = "BuoyancyType == EBuoyancyType::Floating", EditConditionHides, ClampMin = 0.0f))
	float BuoyancyRange = 0.0f;

	FVector GetRandomPointInVolume() const;

public:
	UFUNCTION(BlueprintCallable)
	void SpawnDebris();
};
