#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/I_WeaponInteraction.h"
#include "Components/TimelineComponent.h"
#include "A_PaintableTexture.generated.h"

USTRUCT()
struct FPixelRow
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<int32> Row;
};

UENUM()
enum class ETileType
{
	Floor,
	Wall,
	Air
};

class UNiagaraSystem;

UCLASS()
class ALSEXTRAS_API AA_PaintableTexture : public AActor, public II_WeaponInteraction
{
	GENERATED_BODY()

public:
	AA_PaintableTexture();

protected:
	virtual void OnConstruction(const FTransform& Transform)override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMeshComponent* StaticMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UTexture2D* PaintableTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UTexture* BrushTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UNiagaraSystem* NiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UAudioComponent* AudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (AllowPrivateAccess = true))
	ETileType TileType;

	UPROPERTY()
	UTextureRenderTarget2D* RenderTarget;

	UPROPERTY()
	UMaterialInstanceDynamic* MeshDynamicMaterial;

	UPROPERTY()
	TArray<FPixelRow> PixelArray;

	uint8 bIsOnDissolving : 1{false};

	void ReadTextureToArray();

	void DrawCellOnRenderTarget(int32 CellX, int32 CellY);

	virtual void OnFinish();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* DissolveTimeline;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* DissolveFloatCurve;

	FOnTimelineFloat DissolveProgressFunction;

	FOnTimelineEvent DissolveFinishedFunction;

	UFUNCTION()
	void DissolveTimelineProgress(float Value);

	UFUNCTION()
	void DissolveTimelineFinished();
};
