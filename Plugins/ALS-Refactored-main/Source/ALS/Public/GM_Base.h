#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GM_Base.generated.h"

class AA_EnemyManager;

UCLASS()
class ALS_API AGM_Base : public AGameMode
{
	GENERATED_BODY()

public:
	AGM_Base();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
};
