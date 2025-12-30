#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_Debris.generated.h"

class UNiagaraComponent;

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
protected:
	UPROPERTY(BlueprintReadWrite, Category = "Properties")
	float Alpha = 0.0f;

	UFUNCTION(BlueprintCallable, Category = "Suction")
	void DestroyOrRespawnDebris();

	UFUNCTION(BlueprintCallable, Category = "Suction")
	float DebrisMassInfluence() const;
};
