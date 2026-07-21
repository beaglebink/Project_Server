#pragma once

#include "CoreMinimal.h"
#include "PythonContainers/A_InteractableActor.h"
#include "A_Dishes.generated.h"

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
	USkeletalMeshComponent* AttachedMesh;

	FTimerHandle AttachTimerHandle;
public:
	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void AttachDishToHand(ACharacter* PlayerCharacter, FName SocketName);

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void DetachDishFromHand();

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void RotateDish(float AngleDelta);

	UFUNCTION(BlueprintCallable, Category = "Cooking")
	void TossDish();
};
