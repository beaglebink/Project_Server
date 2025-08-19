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
};
