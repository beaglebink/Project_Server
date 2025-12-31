#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_Debris.generated.h"

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

UCLASS()
class ALSEXTRAS_API AA_Debris : public AActor
{
	GENERATED_BODY()

public:
	AA_Debris();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

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

	void DebrisFloatBehavior(float DeltaTime);

	void DebrisReactToPlayer(float DeltaTime);

private:
	//floating pattern
	FVector BaseLocation;
	FVector InitialDirection;

	float TimeAccumulator = 0.0f;
	float NoiseSeed = 0.0f;

	float OrbitRadius;
	FVector OrbitStartOffset;

	FVector ErraticOffset = FVector::ZeroVector;
public:
	float ErraticRadius = 5.0f;

public:
	//player reaction
	float PlayerInfluenceStrength;
	float PlayerInfluenceRadius;

private:
	float SpeedInfluenceAlpha = 0.0f;
	float CurrentVelocitySize = 0.0f;
	FVector Evasion = FVector::ZeroVector;
	FVector Attraction = FVector::ZeroVector;
	FVector Turbulence = FVector::ZeroVector;
};
