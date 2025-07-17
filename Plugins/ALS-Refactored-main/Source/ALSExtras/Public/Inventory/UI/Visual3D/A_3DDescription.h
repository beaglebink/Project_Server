#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_3DDescription.generated.h"

class USphereComponent;
class USpotLightComponent;
class USceneCaptureComponent2D;

UCLASS()
class ALSEXTRAS_API AA_3DDescription : public AActor
{
	GENERATED_BODY()
	
public:	
	AA_3DDescription();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USceneComponent* SceneComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	USphereComponent* SphereComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* StaticMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USpotLightComponent* SpotLightComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USceneCaptureComponent2D* SceneCaptureComponent2D;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
