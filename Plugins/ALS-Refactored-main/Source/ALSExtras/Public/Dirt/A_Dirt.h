#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/I_WeaponInteraction.h"
#include "A_Dirt.generated.h"

class UBoxComponent;

UCLASS()
class ALSEXTRAS_API AA_Dirt : public AActor, public II_WeaponInteraction
{
	GENERATED_BODY()

public:
	AA_Dirt();

	virtual void OnConstruction(const FTransform& Transform)override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UDecalComponent* DecalComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* BoxComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UTexture2D* DirtTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UMaterialInterface* BrushMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* DecalDynamicMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* BrushDynamicMaterial;

	UPROPERTY()
	UTextureRenderTarget2D* RenderTarget_A;

	UPROPERTY()
	UTextureRenderTarget2D* RenderTarget_B;

private:
	FTimerHandle HitTimerHandle;

	void HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit);
};
