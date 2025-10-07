#pragma once

#include "Components/TimelineComponent.h"
#include "Interfaces\I_WeaponInteraction.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_Rift.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UBoxComponent;

UCLASS()
class ALSEXTRAS_API AA_Rift : public AActor, public II_WeaponInteraction
{
	GENERATED_BODY()

public:
	AA_Rift();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = true))
	UNiagaraComponent* RiftNiagaraComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = true))
	UNiagaraSystem* FiberNiagara;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMesh* HoleStaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UAudioComponent* SewAudioComp;

	UPROPERTY()
	TArray<UBoxComponent*> FromLeftBoxes;

	UPROPERTY()
	TArray<UBoxComponent*> FromRightBoxes;

	UPROPERTY(EditAnywhere, Category = "Setup")
	FVector BoxSize = FVector(2.0f, 20.f, 20.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components|Fiber", meta = (AllowPrivateAccess = true, ClampMin = "0.1", ClampMax = "1.0"))
	float FiberArcTangent = 0.4f;

	UPROPERTY()
	float SpaceBetweenBoxes;

	float FiberSewSpeed = 0.5f;

	uint8 bShouldRotate : 1{false};

	uint8 bCanSew : 1{true};

	int32 Direction;

	int32 PrevIndex = -1;

	FString PrevSide;

	FVector PrevLocation = FVector::ZeroVector;

	UStaticMeshComponent* PrevComponent = nullptr;

	TArray<UStaticMeshComponent*> HoleMeshesArray;

	TArray<UNiagaraComponent*> FiberNiagaraArray;

	virtual void HandleWeaponShot_Implementation(FHitResult& Hit)override;

	virtual void HandleTextFromWeapon_Implementation(const FText& TextCommand)override;

	bool ParseComponentName(const FName& Name, FString& OutSide, int32& OutIndex);

	UStaticMeshComponent* SpawnSewHole(UPrimitiveComponent* Component, FVector HoleLocation);

	void SpawnSeamFiber(UStaticMeshComponent* HoleComp, FVector EndLocation);

	void TightenTheSeam();

	FVector GetCornerOffset(int32 XSign, int32 YSign, float Offset);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* SewTimeline;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* SewFloatCurve;

	FOnTimelineFloat SewProgressFunction;

	FOnTimelineEvent SewFinishedFunction;

	UFUNCTION()
	void SewTimelineProgress(float Value);

	UFUNCTION()
	void SewTimelineFinished();
};
