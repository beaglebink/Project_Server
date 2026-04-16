#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GM_Base.generated.h"

// ƒелегаты объ€влены здесь Ч плагин не зависит от игрового модул€
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLevelLoadingStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLevelLoadingFinished);

class AA_EnemyManager;

UCLASS()
class ALS_API AGM_Base : public AGameMode
{
	GENERATED_BODY()

public:
	AGM_Base();

	virtual void PostSeamlessTravel() override;
	virtual void GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "GameMode|Loading")
	void GetAdditionalSeamlessTravelActors(TArray<AActor*>& OutActors);

	UFUNCTION(BlueprintCallable, Category = "GameMode|Loading")
	void NotifyLoadingStarted();

	UFUNCTION(BlueprintCallable, Category = "GameMode|Loading")
	void NotifyLoadingFinished();

	UPROPERTY(BlueprintAssignable, Category = "GameMode|Loading")
	FOnLevelLoadingStarted OnLevelLoadingStarted;

	UPROPERTY(BlueprintAssignable, Category = "GameMode|Loading")
	FOnLevelLoadingFinished OnLevelLoadingFinished;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GameMode|Loading")
	bool IsSpawnPlayer = true;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
};
