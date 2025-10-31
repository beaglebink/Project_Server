#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/I_WeaponInteraction.h"
#include "Components/TimelineComponent.h"
#include "A_BlockObstacle.generated.h"

UENUM(BlueprintType)
enum class EBlockState :uint8
{
	Neutral,
	Critical,
	Destroyed,
	Moving,
	Falling,
	Shot
};

UENUM(BlueprintType)
enum class EDirection :uint8
{
	Left,
	Right,
	Down,
	Up
};

class UNiagaraComponent;
class UAudioComponent;

UCLASS()
class ALSEXTRAS_API AA_BlockObstacle : public AActor, public II_WeaponInteraction
{
	GENERATED_BODY()

public:
	AA_BlockObstacle();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	EBlockState BlockState = EBlockState::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	TObjectPtr<UStaticMeshComponent> BlockMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	UAudioComponent* AudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	USoundBase* BlockDestroySound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	USoundBase* BlockMoveSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	USoundBase* BlockFallSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	UNiagaraComponent* BlockDestroyFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	UMaterialInterface* BlockMaterialAsset;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BlockMaterialInstance;

	UPROPERTY()
	FVector InitialLocation;

	UPROPERTY()
	FVector TargetLocation;

	UPROPERTY()
	int32 BlockIndex;

	void SetBlockState(EBlockState NewState);

	void SetBlockMaterial(bool bIsCritical);

	void OnShotMaterial(bool bIsShot);

	void StartBlockDestroy();

	void MoveBlock(EDirection Direction);

	void StartBlockFall();

private:

	void HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit);

protected:
	//Destroy Timeline
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* DestroyTimeline;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* DestroyFloatCurve;

	FOnTimelineFloat DestroyProgressFunction;

	FOnTimelineEvent DestroyFinishedFunction;

	UFUNCTION()
	void DestroyTimelineProgress(float Value);

	UFUNCTION()
	void DestroyTimelineFinished();

	//Move Timeline
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* MoveBlockTimeline;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* MoveBlockFloatCurve;

	FOnTimelineFloat MoveBlockProgressFunction;

	FOnTimelineEvent MoveBlockFinishedFunction;

	UFUNCTION()
	void MoveBlockTimelineProgress(float Value);

	UFUNCTION()
	void MoveBlockTimelineFinished();

	//Fall Timeline
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* FallBlockTimeline;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* FallBlockFloatCurve;

	FOnTimelineFloat FallBlockProgressFunction;

	FOnTimelineEvent FallBlockFinishedFunction;

	UFUNCTION()
	void FallBlockTimelineProgress(float Value);

	UFUNCTION()
	void FallBlockTimelineFinished();
};
