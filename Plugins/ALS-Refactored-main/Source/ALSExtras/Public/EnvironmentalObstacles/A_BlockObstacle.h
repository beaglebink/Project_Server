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
	Destroyed
};

UENUM(BlueprintType)
enum class EDirection :uint8
{
	Up,
	Right,
	Down,
	Left
};

class UNiagaraComponent;
class UAudioComponent;
class AA_BlocksWallObstacle;

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
	USoundBase* BlockMoveOnDirectionSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	USoundBase* BlockFallSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	UNiagaraComponent* BlockDestroyFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	UMaterialInterface* BlockMaterialAsset;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BlockMaterialInstance;

	UPROPERTY()
	FVector BlockExtent;

	UPROPERTY()
	FVector InitialLocation;

	UPROPERTY()
	FVector TargetLocation;

	UPROPERTY()
	int32 BlockIndex;

	UPROPERTY()
	AA_BlocksWallObstacle* GridOfBlocks;

	UPROPERTY()
	uint8 bIsPulling : 1{ true };

	UPROPERTY()
	uint8 bIsFalling : 1{ false };
	
	UPROPERTY()
	float FallSpeed = 0.0f;

	UPROPERTY()
	AA_BlockObstacle* LowerBlock;

	void SetBlockState(EBlockState NewState);

	void SetBlockMaterial(bool bIsCritical);

	void HandleShotForMaterial();

	void OnShotMaterial(bool bIsShot);

	void StartBlockDestroy();

	void MoveOnDirectionBlock(EDirection Direction);

	void StartBlockFall(AA_BlockObstacle* Block);

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

	//MoveOnDirection Timeline
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* MoveOnDirectionBlockTimeline;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* MoveOnDirectionBlockFloatCurve;

	FOnTimelineFloat MoveOnDirectionBlockProgressFunction;

	FOnTimelineEvent MoveOnDirectionBlockFinishedFunction;

	UFUNCTION()
	void MoveOnDirectionBlockTimelineProgress(float Value);

	UFUNCTION()
	void MoveOnDirectionBlockTimelineFinished();

	//FrontBack Timeline
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* FrontBackBlockTimeline;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* FrontBackBlockFloatCurve;

	FOnTimelineFloat FrontBackBlockProgressFunction;

	FOnTimelineEvent FrontBackBlockFinishedFunction;

	UFUNCTION()
	void FrontBackBlockTimelineProgress(float Value);

	UFUNCTION()
	void FrontBackBlockTimelineFinished();
};
