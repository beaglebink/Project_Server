#pragma once

#include "Components/TimelineComponent.h"
#include "Interfaces/I_WeaponInteraction.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_Portal.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UBoxComponent;
class UInteractiveItemComponent;

UCLASS()
class ALSEXTRAS_API AA_Portal : public AActor, public II_WeaponInteraction
{
	GENERATED_BODY()

public:
	AA_Portal();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "30"))
	float CoolDownTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* PortalMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneCaptureComponent2D* PortalCaptureComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UTextureRenderTarget2D* PortalRenderTarget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* PortalTriggerComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UAudioComponent* PortalAudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* PortalButtonMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UInteractiveItemComponent* PortalInteractiveComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AA_Portal> ExitPortal;

	UPROPERTY()
	UMaterialInstanceDynamic* PortalDynamicMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* PortalButtonDynamicMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Activation", meta = (AllowPrivateAccess = "true"))
	uint8 bIsActive : 1{false};

	void CameraFollowsCharacterView();

	UFUNCTION()
	void PortalOnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	virtual void HandleWeaponShot_Implementation(FHitResult& Hit) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* ActivationTimeline;

	UPROPERTY(EditAnywhere, Category = "Timeline")
	UCurveFloat* ActivationFloatCurve;

	FOnTimelineFloat ActivationProgressFunction;

	FOnTimelineEvent ActivationFinishedFunction;

	UFUNCTION()
	void ActivationTimelineProgress(float Value);

	UFUNCTION()
	void ActivationTimelineFinished();

	UFUNCTION()
	void StartActivateDeactivatePortal(UInteractivePickerComponent* Picker);
};
