#pragma once

#include "CoreMinimal.h"
#include "PythonContainers/A_InteractableActor.h"
#include "A_Breakable.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API AA_Breakable : public AA_InteractableActor
{
	GENERATED_BODY()

public:
	AA_Breakable();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Destroyed() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breakable")
	UGeometryCollectionComponent* GeometryCollectionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breakable", meta = (ClampMin = 0.0f))
	float BreakImpulseThreshold = 10.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Breakable")
	uint8 bIsBroken : 1{false};

	void BreakActor();

	void RestoreActor();

	UFUNCTION()
	void OnActorHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

};
