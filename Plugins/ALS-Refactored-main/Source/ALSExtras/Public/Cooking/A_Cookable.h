#pragma once

#include "CoreMinimal.h"
#include "PythonContainers/A_InteractableActor.h"
#include "Components/TimelineComponent.h"
#include "Cooking/DA_Recipes.h"
#include "A_Cookable.generated.h"

UENUM(BlueprintType)
enum class EDishRating : uint8
{
	Poor		UMETA(DisplayName = "Poor"),
	Acceptable	UMETA(DisplayName = "Acceptable"),
	Good		UMETA(DisplayName = "Good"),
	Excellent	UMETA(DisplayName = "Excellent")
};

class UProceduralMeshComponent;
class AA_Dishes;
class UTextRenderComponent;

UCLASS()
class ALSEXTRAS_API AA_Cookable : public AA_InteractableActor
{
	GENERATED_BODY()

public:
	AA_Cookable();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	virtual void Destroyed() override;

	virtual void HandleCutting_Implementation(UPARAM(ref)FHitResult& Hit, FVector CutPlaneNormal) override;

	void CopyProceduralMesh(UProceduralMeshComponent* Source, UProceduralMeshComponent* Target);

	void BuildConvexCollision(UProceduralMeshComponent* Mesh);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slicing mesh")
	UProceduralMeshComponent* SlicedMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rating")
	UTextRenderComponent* DishRatingText;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0))
	int32 DefaultCookingTime = 60;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	int32 CookingTime = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	float Doneness;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float IdealDoneness = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.1f, ClampMax = 1.0f))
	float UndercookTolerance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.1f, ClampMax = 1.0f))
	float OvercookTolerance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float CookRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 2.0f))
	float RecipeImportance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float CriticalRawThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float CriticalBurnThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float WorstChunkInfluence = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float MinimumSignificantChunkSize;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CookingSettings", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float Quality = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	float ChunkMass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	AA_Dishes* PrevParentDish = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	uint8 bIsInsideADish : 1{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CookingSettings")
	uint8 bWasTossed : 1{false};

	UPROPERTY()
	UMaterialInstanceDynamic* MeshDynamicMaterial;

	void IncreaseCookingTime(int32 CookingPeriod);

	void DistributeMassesByCut(AA_Cookable* CutPart);

	float GetChunkQuality();

	EDishRating GetDishRating(float DishQuality, const FDishQualityThresholds& Thresholds);

	void ShowFinalDishRating(EDishRating DishRating);
};
