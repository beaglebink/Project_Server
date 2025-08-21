#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_DiscreteSystemConveyor.generated.h"

class AA_DiscreteSystemNode;

UCLASS()
class ALSEXTRAS_API AA_DiscreteSystemConveyor : public AActor
{
	GENERATED_BODY()

public:
	AA_DiscreteSystemConveyor();

protected:
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TArray<AA_DiscreteSystemNode*> NodeActors;

	UPROPERTY()
	TArray<int32> NodeOrder;

	int32 CurrentNodeIndex = 0;

	UFUNCTION()
	void OnNodeLogicFinished();

	UFUNCTION()
	void OrderCorrection(int32 NodeNumberDefault, int32 NodeNumber);

	void ArraysInitialization();

public:
	UFUNCTION(BlueprintCallable, Category = "SystemLogic")
	void StartSystemLogic();

	UFUNCTION(BlueprintCallable, Category = "SystemLogic")
	void StopSystemLogic();

	UFUNCTION(BlueprintCallable, Category = "SystemLogic")
	void ShuffleSystemLogic();
};
