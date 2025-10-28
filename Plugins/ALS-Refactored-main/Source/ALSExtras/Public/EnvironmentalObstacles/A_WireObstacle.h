#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/I_WeaponInteraction.h"
#include "Components/TimelineComponent.h"
#include "A_WireObstacle.generated.h"

class UBoxComponent;
class USplineComponent;
class USplineMeshComponent;
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class ALSEXTRAS_API AA_WireObstacle : public AActor, public II_WeaponInteraction
{
	GENERATED_BODY()

public:
	AA_WireObstacle();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UBoxComponent* BoxComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UAudioComponent* AudioComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMesh* NodeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UNiagaraSystem* WireFX;

	UPROPERTY()
	UMaterialInterface* WireMaterial;

	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters", meta = (AllowPrivateAccess = true, ClampMin = "0"))
	float WireObstacleRadius = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters", meta = (AllowPrivateAccess = true, ClampMin = "0"))
	int32 AnchorsPerSide = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters", meta = (AllowPrivateAccess = true, ClampMin = "1", ClampMax = "10"))
	int32 NodesQuantity = 5;

	UPROPERTY(VisibleAnywhere, Category = "Wires|Debug")
	TArray<FVector> LeftAnchors;

	UPROPERTY(VisibleAnywhere, Category = "Wires|Debug")
	TArray<FVector> RightAnchors;

	UPROPERTY(VisibleAnywhere, Category = "Wires|Debug")
	TArray<FVector> LeftHitPoints;

	UPROPERTY(VisibleAnywhere, Category = "Wires|Debug")
	TArray<FVector> RightHitPoints;

	UPROPERTY(VisibleAnywhere, Category = "Wires|Debug")
	TArray<USplineComponent*> WireSplines;

	UPROPERTY(VisibleAnywhere, Category = "Wires|Nodes")
	TArray<UPrimitiveComponent*> Nodes;

	UPROPERTY(VisibleAnywhere, Category = "Wires")
	TArray<UNiagaraComponent*> WireFXArray;

	template<typename T>
	void ShuffleArray(TArray<T>& Array);

	void HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* RemoveWireTimeline;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* RemoveWireFloatCurve;

	FOnTimelineFloat RemoveWireProgressFunction;

	FOnTimelineEvent RemoveWireFinishedFunction;

	UFUNCTION()
	void RemoveWireTimelineProgress(float Value);

	UFUNCTION()
	void RemoveWireTimelineFinished();
};
