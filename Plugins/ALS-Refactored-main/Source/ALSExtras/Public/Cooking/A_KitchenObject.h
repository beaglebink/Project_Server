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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMeshComponent* StaticMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UBoxComponent* SurfaceCollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking", meta = (AllowPrivateAccess = true))
	TArray<AA_Dishes*> PlacedDishes;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooking")
	EHeatingLevel HeatingLevel;
};
