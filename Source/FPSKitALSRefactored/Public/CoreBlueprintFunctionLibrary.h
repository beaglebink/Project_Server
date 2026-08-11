#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputTriggers.h"
#include "InputModifiers.h"
#include "CoreBlueprintFunctionLibrary.generated.h"

USTRUCT(BlueprintType)
struct FFluidPoints
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fluid Points")
	TArray<FVector> LocalPositions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fluid Points")
	TArray<float> DistancesToWall;
};

UCLASS()
class FPSKITALSREFACTORED_API UCoreBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Input")
	static void SimulateKeyPress(APlayerController* PlayerController, UInputAction* InputAction);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "General")
	static bool IsPIE();

	UFUNCTION(BlueprintCallable, Category = "Objects")
	static USceneComponent* GetSceneComponentCopy(USceneComponent* Object, AActor* Outer, FText Name);

	UFUNCTION(BlueprintCallable, Category = "Objects")
	static void RemoveSceneComponent(USceneComponent* Component);

	//Fluid points grid calculation inside mesh volume
	UFUNCTION(BlueprintCallable, Category = "Fluid Points")
	static FFluidPoints GenerateFluidPointsGrid(UStaticMeshComponent* MeshComponent, float PointSpacing, bool bDrawDebug);
};

