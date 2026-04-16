#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GM_Base.h" // делегаты берЄм отсюда
#include "FPSKitGameMode.generated.h"

// FOnLevelLoadingStarted и FOnLevelLoadingFinished объ€влены в GM_Base.h Ч не дублируем

UCLASS()
class FPSKITALSREFACTORED_API AFPSKitGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPSKitGameMode();

	virtual void PostSeamlessTravel() override;

	virtual void GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "FPSKit|Loading")
	void GetAdditionalSeamlessTravelActors(TArray<AActor*>& OutActors);

	UFUNCTION(BlueprintCallable, Category = "FPSKit|Loading")
	void NotifyLoadingStarted();

	UFUNCTION(BlueprintCallable, Category = "FPSKit|Loading")
	void NotifyLoadingFinished();

	UPROPERTY(BlueprintAssignable, Category = "FPSKit|Loading")
	FOnLevelLoadingStarted OnLevelLoadingStarted;

	UPROPERTY(BlueprintAssignable, Category = "FPSKit|Loading")
	FOnLevelLoadingFinished OnLevelLoadingFinished;
};