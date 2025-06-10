#pragma once

#include "GameplayTagContainer.h"
#include "Components/TimelineComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "P_Bubble.generated.h"

class USphereComponent;
class UFloatingPawnMovement;
class AAlsCharacterExample;

UCLASS()
class ALSEXTRAS_API AP_Bubble : public APawn
{
	GENERATED_BODY()

public:
	AP_Bubble();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* SphereCollisionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* StaticMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UFloatingPawnMovement* FloatingPawnMovementComp;

	UPROPERTY(BlueprintReadWrite, Category = "DynamicMaterial")
	UMaterialInstanceDynamic* DynMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EffectTime")
	float Time;

	UPROPERTY(BlueprintReadWrite, Category = "CharacterRef")
	AAlsCharacterExample* CatchedCharacter;

	UPROPERTY(EditDefaultsOnly, Category = "Timeline")
	UCurveFloat* FloatCurve;

private:
	UPROPERTY()
	UTimelineComponent* CatchTimeline;

	float CharacterGravity;

	FGameplayTag PrevViewTag;

	float DistanceMeshToCollision;

	uint8 bIsCatched : 1{false};

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	FOnTimelineFloat ProgressFunction;

	FOnTimelineEvent FinishedFunction;

	UFUNCTION()
	void TimelineProgress(float Value);

	UFUNCTION()
	void TimelineFinished();
};
