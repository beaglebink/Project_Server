#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_Scanner.generated.h"

UCLASS()
class ALSEXTRAS_API AA_Scanner : public AActor
{
	GENERATED_BODY()
	
public:	
	AA_Scanner();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

};
