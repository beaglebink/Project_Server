#pragma once

#include "Components/TimelineComponent.h"
#include "Interfaces\I_WeaponInteraction.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_HazardActor.generated.h"

UCLASS()
class ALSEXTRAS_API AA_HazardActor : public AActor, public II_WeaponInteraction
{
	GENERATED_BODY()

public:
	AA_HazardActor();

protected:
	virtual void OnConstruction(const FTransform& Transform)override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* StaticMeshComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UAudioComponent* AudioComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USoundBase* HitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USoundBase* DeathSound;

	UPROPERTY()
	UMaterialInstanceDynamic* MeshDynamicMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = "true", ClampMin = "1", ClampMax = "9"))
	int32 DamageCaused = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "15"))
	float BaseAmplitude;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "15"))
	float BaseFrequency;

	UPROPERTY()
	FVector DefaultLocation;

	FVector RandomAmplitude;
	
	FVector RandomFrequency;

	virtual void HandleWeaponShot_Implementation(FHitResult& Hit)override;

	void OnDeath();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* DeathTimeline;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* DeathFloatCurve;

	FOnTimelineFloat DeathProgressFunction;

	FOnTimelineEvent DeathFinishedFunction;

	UFUNCTION()
	void OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult);

	UFUNCTION()
	void DeathTimelineProgress(float Value);

	UFUNCTION()
	void DeathTimelineFinished();
};
