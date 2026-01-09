#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_Debris.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDebrisDestroyed, FTransform, DebrisTransform, FVector, DebrisBoxExtent, int8, DestructionLevel);

class UNiagaraComponent;

UENUM(BlueprintType)
enum class EDebrisFloatPattern : uint8
{
	Stationary,
	Drift,
	Wander,
	Orbit,
	Erratic
};

UENUM(BlueprintType)
enum class EDebrisReactToPlayer : uint8
{
	Passive,
	Evasive,
	Attracted,
	Neutral
};

UENUM(BlueprintType)
enum class EBuoyancyType : uint8
{
	Fixed,
	Floating,
	Rising,
	Sinking
};

UENUM(BlueprintType)
enum class EDebrisCollisionReact : uint8
{
	PhaseThrough,
	Bounce,
	Stick,
	Stop
};

UENUM(BlueprintType)
enum class EDebrisDestructionYield : uint8
{
	None,
	Light,
	Medium,
	Heavy,
	Chain
};

UCLASS()
class ALSEXTRAS_API AA_Debris : public AActor
{
	GENERATED_BODY()

public:
	AA_Debris();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

	virtual void Destroyed() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMeshComponent* DebrisMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UNiagaraComponent* DebrisFlowFXComponent;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MeshMaterialInstanceDynamic;

	UPROPERTY()
	UTextureRenderTarget2D* MeshRenderTarget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components|VFX", meta = (AllowPrivateAccess = true))
	UMaterialInterface* ParticlesMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = true))
	TSubclassOf<AA_Debris> DebrisClass;

	FTimerHandle RespawnTimerHandle;

public:
	uint8 bShouldMeshSimulatePhysics : 1{false};

	float ReplenishTime = 0.0f;

	float SuctionDifficultyMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Properties")
	float Slipperiness = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Properties")
	float SlipperinessImpulsePower = 500.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Properties")
	float AttachmentPower = 0.0f;

	FVector AttachmentDirection = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Properties")
	float GarbagePercentage = 0.0f;

	EDebrisFloatPattern DebrisBehavior = EDebrisFloatPattern::Stationary;

	float DebrisMovementSpeed;

	EDebrisReactToPlayer PlayerReaction = EDebrisReactToPlayer::Passive;

protected:
	UPROPERTY(BlueprintReadWrite, Category = "Properties")
	float Alpha = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Suction")
	void DestroyOrRespawnDebris();

	UFUNCTION(BlueprintCallable, Category = "Suction")
	float DebrisMassInfluence() const;

	UFUNCTION(BlueprintCallable, Category = "Suction")
	void CheckIfDebrisDetached(FVector DeltaForDetachment, float Effectiveness);

	void DebrisFloatBehavior(float DeltaTime);

	void DebrisReactToPlayer(float DeltaTime);

	//floating pattern
private:
	FVector BaseLocation;
	FVector TargetLocation;
	FVector InitialDirection;

	float TimeAccumulator = 0.0f;
	float NoiseSeed = 0.0f;

	float OrbitRadius;
	FVector OrbitStartOffset;

	FVector ErraticOffset = FVector::ZeroVector;
public:
	float ErraticRadius = 5.0f;

	//player reaction
public:
	float PlayerInfluenceStrength;
	float PlayerInfluenceRadius;

private:
	float SpeedInfluenceAlpha = 0.0f;
	float CurrentVelocitySize = 0.0f;
	FVector Evasion = FVector::ZeroVector;
	FVector Attraction = FVector::ZeroVector;
	FVector Turbulence = FVector::ZeroVector;

	//buoyancy
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Properties")
	EBuoyancyType BuoyancyType = EBuoyancyType::Fixed;
	float BuoyancySpeed = 0.0f;
	float BuoyancyRange = 0.0f;
	float BuoyancyDistanceAccumulator = 0.0f;
	float BuoyancyDirection = 1.0f;

private:
	void DebrisBuoyancyBehavior(float DeltaTime);

	//collision
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Properties")
	EDebrisCollisionReact CollisionReaction = EDebrisCollisionReact::PhaseThrough;

private:
	uint8 bHasCollision : 1{false};
	uint8 bHasStuckToSurface : 1{false};
	uint8 bIsStopped : 1{false};

	FVector PreviousLocation = FVector::ZeroVector;
	FVector CurrentLocation = FVector::ZeroVector;
	FVector CurrentVelocity = FVector::ZeroVector;

	AActor* StuckActor = nullptr;
	FTransform StuckTransformOffset = FTransform::Identity;

	void FollowStuckActor();

	UFUNCTION()
	void OnDebrisHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	//destruction
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Properties")
	float DebrisHealth = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Properties")
	EDebrisDestructionYield DestructionYield = EDebrisDestructionYield::None;

	FOnDebrisDestroyed OnDebrisDestroyed;

	int8 DestructionLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Properties")
	float ChainReactionSphereRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Properties")
	float ChainReactionDamageAmount = 0.0f;

private:
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

};
