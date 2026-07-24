#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_KitchenObject.generated.h"

class UBoxComponent;

UCLASS()
class ALSEXTRAS_API AA_KitchenObject : public AActor
{
	GENERATED_BODY()
	
public:	
	AA_KitchenObject();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	virtual void Destroyed() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* StaticMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* SurfaceCollisionComponent;
};
