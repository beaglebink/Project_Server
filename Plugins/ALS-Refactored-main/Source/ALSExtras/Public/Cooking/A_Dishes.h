#pragma once

#include "CoreMinimal.h"
#include "PythonContainers/A_InteractableActor.h"
#include "A_Dishes.generated.h"

class USphereComponent;
class AA_Cookable;

UCLASS()
class ALSEXTRAS_API AA_Dishes : public AA_InteractableActor
{
	GENERATED_BODY()


public:
	AA_Dishes();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	virtual void Destroyed() override;

private:
	uint8 bIsOnAttaching : 1{false};

	UPROPERTY()
	TArray<AA_Cookable*> CookableIngredients;

	UPROPERTY()
	TMap<FName, int32> IngredientCountMap;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* CollisionShape;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking")
	USkeletalMeshComponent* AttachedMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking")
	FName AttachedSocketName;

	FTimerHandle AttachTimerHandle;
	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void AttachDishToHand(ACharacter* PlayerCharacter, FName SocketName);

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void DetachDishFromHand();

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void RotateDish(float AngleDelta);

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void TossDish();

	UFUNCTION()
	void OnSphereOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
