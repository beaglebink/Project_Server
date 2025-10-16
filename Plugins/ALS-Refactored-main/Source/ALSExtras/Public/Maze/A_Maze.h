#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/I_WeaponInteraction.h"
#include "A_Maze.generated.h"

USTRUCT()
struct FMazeRow
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<int32> Row;
};

UCLASS()
class ALSEXTRAS_API AA_Maze : public AActor, public II_WeaponInteraction
{
	GENERATED_BODY()

public:
	AA_Maze();

protected:
	virtual void OnConstruction(const FTransform& Transform)override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMeshComponent* StaticMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UTexture2D* MazeTexture;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UTextureRenderTarget2D* MazeRenderTarget;

	UPROPERTY()
	UMaterialInstanceDynamic* MeshDynamicMaterial;

	UPROPERTY()
	TArray<FMazeRow> MazeArray;

	void ReadMazeTextureToArray();

	void HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit);
};
