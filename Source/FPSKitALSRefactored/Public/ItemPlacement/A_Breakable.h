#pragma once

#include "CoreMinimal.h"
#include "PythonContainers/A_InteractableActor.h"
#include "A_Breakable.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBreakableBroken, AA_Breakable*, BreakableActor);


class UFloorAssignmentComponent;
class UGeometryCollection;

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UFloorAssignmentComponent* FloorAssignmentComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UGeometryCollectionComponent* GeometryCollectionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UGeometryCollection* DefaultGeometryCollection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UAudioComponent* AudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breakable", meta = (ClampMin = 0.0f))
	float BreakImpulseThreshold = 1000.0f;

	UPROPERTY(BlueprintAssignable, Category = "Breakable")
	FOnBreakableBroken OnBroken;

	void BreakActor();

	UFUNCTION(BlueprintCallable, Category = "Breakable")
	void SetIsBroken(bool IsBroken);

	UFUNCTION(BlueprintCallable, Category = "Breakable")
	bool GetIsBroken()const;

	void RestoreActor();

	UFUNCTION()
	void OnActorHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	UPROPERTY()
	FTransform DefaultTransform;

	UPROPERTY()
	uint8 bIsBroken : 1{false};
};
