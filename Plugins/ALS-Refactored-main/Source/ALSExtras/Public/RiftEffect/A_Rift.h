#pragma once

#include "Components/TimelineComponent.h"
#include "Interfaces\I_WeaponInteraction.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_Rift.generated.h"

class UNiagaraComponent;
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
	UNiagaraComponent* SeamNiagaraComp;

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

	UPROPERTY()
	float SpaceBetweenBoxes;

	int32 Direction;

	int32 PrevIndex = -1;

	FString PrevSide;

	FVector PrevLocation = FVector::ZeroVector;

	virtual void HandleWeaponShot_Implementation(FHitResult& Hit)override;

	virtual void HandleTextFromWeapon_Implementation(const FText& TextCommand)override;

	bool ParceComponentName(const FName& Name, FString& OutSide, int32& OutIndex);

	void SpawnSewHole(FVector HoleLocation);

	void SpawnSeam(FVector StartLocation, FVector EndLocation);

	void TightenTheSeam(FVector Location);

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
