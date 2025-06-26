#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_PickUp.generated.h"

UCLASS()
class ALSEXTRAS_API AA_PickUp : public AActor
{
	GENERATED_BODY()
	
public:	
	AA_PickUp();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
