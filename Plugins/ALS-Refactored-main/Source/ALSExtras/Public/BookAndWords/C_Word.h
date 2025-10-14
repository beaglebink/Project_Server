#pragma once

#include "CoreMinimal.h"
#include "AlsCharacterExample.h"
#include "Interfaces/I_WeaponInteraction.h"
#include "Components/TimelineComponent.h"
#include "C_Word.generated.h"

UCLASS()
class ALSEXTRAS_API AC_Word : public AAlsCharacterExample, public II_WeaponInteraction
{
	GENERATED_BODY()

public:
	AC_Word();

protected:
	virtual void OnConstruction(const FTransform& Transform)override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UAudioComponent* AudioComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components|Property", meta = (AllowPrivateAccess = true))
	FText Word;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components|Property", meta = (AllowPrivateAccess = true))
	int32 BookGroupCode = -1;

	UPROPERTY()
	FVector InitialLocation;

	UPROPERTY()
	FVector FloatAmplitude;

	UPROPERTY()
	FVector FloatFrequency;

	UPROPERTY()
	FVector FloatPhase;

	UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	uint8 bIsDragged : 1{false};

	uint8 bIsAbsorbing : 1 {false};

	FVector DefaultWordLocation;

	FVector TargetBookLocation;

	UFUNCTION()
	void OnLetterBeginOverlap(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void Absorbing();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* AbsorbTimeline;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* AbsorbFloatCurve;

	FOnTimelineFloat AbsorbProgressFunction;

	FOnTimelineEvent AbsorbFinishedFunction;

	UFUNCTION()
	void AbsorbTimelineProgress(float Value);

	UFUNCTION()
	void AbsorbTimelineFinished();

	void HandleWeaponDrag_Implementation(bool bIsDragging);
};
