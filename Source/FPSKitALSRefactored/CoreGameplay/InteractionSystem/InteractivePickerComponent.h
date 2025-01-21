// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "Components/ActorComponent.h"
#include "InteractiveItemComponent.h"
#include "InteractivePickerComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FPSKITALSREFACTORED_API UInteractivePickerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInteractivePickerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	FORCEINLINE float GetPickRadius() const { return PickRadius; }

private:
	void TickPicker(float DeltaTime);

	UInteractiveItemComponent* TraceNearestUsableObject(const FVector& Location, const FVector& Direction, TMap<AActor*, FString>& OutTracedActors, float Radius,
		const TArray<AActor*>& ActorsToIgnore) const;

public:
	UPROPERTY(Category = "TheGame|InteractiveItem", EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	bool DebugDraw = true;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	bool IsShowInteractionTrace = false;

	UPROPERTY(Category = "TheGame|InteractiveItem", EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float PickTickInterval = 0.3f;

	UPROPERTY(Category = "TheGame|InteractiveItem", EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float PickRadius = 300.f;

	UPROPERTY(Category = "TheGame|InteractiveItem", EditDefaultsOnly, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	float CapsuleRadius = 35.f;

private:
	FTimerDelegate TimerDel;
	FTimerHandle TimerHandle;

	UPROPERTY()
	TMap<AActor*, FString> TracedActors;

	UPROPERTY()
	TArray<AActor*> FoundCharacters;

	UPROPERTY()
	TArray<AActor*> ActorsToIgnoreCache;
		
};
