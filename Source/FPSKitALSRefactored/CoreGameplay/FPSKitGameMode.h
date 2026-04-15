#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FPSKitGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLevelLoadingStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLevelLoadingFinished);

UCLASS()
class FPSKITALSREFACTORED_API AFPSKitGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPSKitGameMode();

	virtual void PostSeamlessTravel() override;

	// Указываем движку каких акторов НЕ разрушать при SeamlessTravel
	virtual void GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList) override;

	UFUNCTION(BlueprintCallable, Category = "FPSKit|Loading")
	void NotifyLoadingStarted();

	UFUNCTION(BlueprintCallable, Category = "FPSKit|Loading")
	void NotifyLoadingFinished();

	UPROPERTY(BlueprintAssignable, Category = "FPSKit|Loading")
	FOnLevelLoadingStarted OnLevelLoadingStarted;

	UPROPERTY(BlueprintAssignable, Category = "FPSKit|Loading")
	FOnLevelLoadingFinished OnLevelLoadingFinished;
};