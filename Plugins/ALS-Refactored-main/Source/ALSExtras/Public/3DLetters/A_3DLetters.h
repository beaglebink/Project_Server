#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/I_WeaponInteraction.h"
#include "Components/TimelineComponent.h"
#include "A_3DLetters.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FLetter
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> LetterMeshComponent;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> LetterDestroyFX;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> LetterMaterialInstanceDynamic;

	UPROPERTY()
	FVector InitialLocation;
	
	UPROPERTY()
	FVector TargetLocation;

	UPROPERTY()
	FName LetterChar;

	UPROPERTY()
	float FloatAmplitude;

	UPROPERTY()
	float FloatSpeed;

	UPROPERTY()
	float FloatPhase;
};

UCLASS()
class ALSEXTRAS_API AA_3DLetters : public AActor, public II_WeaponInteraction
{
	GENERATED_BODY()
	
public:	
	AA_3DLetters();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMeshComponent* LettersToMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UAudioComponent* AudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	USoundBase* SwapSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	USoundBase* TransformSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	FString LettersText;

	UPROPERTY()
	FString CurrentLettersText;

	FString CommandLettersText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	UNiagaraSystem* LetterDestroyFXSystem;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true, ClampMin = "0"))
	float Spacing = 40.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Settings", meta = (AllowPrivateAccess = true))
	TArray<TObjectPtr<UStaticMesh>> LetterMeshes;

	UPROPERTY()
	TArray<FLetter> LettersArray;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MeshMaterialInstanceDynamic;

	uint8 bIsSwappingLetters : 1{ false };

	uint8 bIsTransformingLettersToMesh : 1{ false };

	void HandleTextFromWeapon_Implementation(const FText& TextCommand);

	bool AreWordsEqualIgnoreOrder(const FString& A, const FString& B);

	void StartTransformLettersToMesh();

protected:
	//SwapLetters Timeline
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* SwapLettersTimeline;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* SwapLettersFloatCurve;

	FOnTimelineFloat SwapLettersProgressFunction;

	FOnTimelineEvent SwapLettersFinishedFunction;

	UFUNCTION()
	void SwapLettersTimelineProgress(float Value);

	UFUNCTION()
	void SwapLettersTimelineFinished();

	//TransformLettersToMesh Timeline
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* TransformLettersToMeshTimeline;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* TransformLettersToMeshFloatCurve;

	FOnTimelineFloat TransformLettersToMeshProgressFunction;

	FOnTimelineEvent TransformLettersToMeshFinishedFunction;

	UFUNCTION()
	void TransformLettersToMeshTimelineProgress(float Value);

	UFUNCTION()
	void TransformLettersToMeshTimelineFinished();
};
