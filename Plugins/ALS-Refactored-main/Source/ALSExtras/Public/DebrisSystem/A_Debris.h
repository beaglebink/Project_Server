#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_Debris.generated.h"

class UNiagaraSystem;

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
};
