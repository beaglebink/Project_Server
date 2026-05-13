#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/I_WeaponInteraction.h"
#include "AC_Dirt.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALSEXTRAS_API UAC_Dirt : public UActorComponent, public II_WeaponInteraction
{
	GENERATED_BODY()

public:	
	UAC_Dirt();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UTexture2D* DirtTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UMaterialInterface* BrushMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* OverlayDynamicMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* BrushDynamicMaterial;

	UPROPERTY()
	UTextureRenderTarget2D* RenderTarget_A;

	UPROPERTY()
	UTextureRenderTarget2D* RenderTarget_B;

	UPROPERTY()
	UStaticMeshComponent* StaticMeshComponent;

private:
	void HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit);
};
