#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_DiscreteSystemConveyor.generated.h"

UCLASS()
class ALSEXTRAS_API AA_DiscreteSystemConveyor : public AActor
{
	GENERATED_BODY()
	
public:	
	AA_DiscreteSystemConveyor();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TArray<UChildActorComponent*> NodeActors;

	UPROPERTY(BlueprintReadWrite, Category = "OrderNumbers", meta = (AllowPrivateAccess = "true"))
	TArray<int32> NodeOrder;

	int32 CurrentNodeIndex = 0;

	UFUNCTION()
	void OnNodeLogicFinished();

public:
	UFUNCTION(BlueprintCallable, Category = "SystemLogic")
	void StartSystemLogic();

	UFUNCTION(BlueprintCallable, Category = "SystemLogic")
	void ShuffleSystemLogic();
};
