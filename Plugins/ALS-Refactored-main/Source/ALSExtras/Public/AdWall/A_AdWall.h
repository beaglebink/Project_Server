#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_AdWall.generated.h"

UCLASS()
class ALSEXTRAS_API AA_AdWall : public AActor
{
	GENERATED_BODY()
	
public:	
	AA_AdWall();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
