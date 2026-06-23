#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TimelineComponent.h"
#include "NativeGameplayTags.h"
#include "A_Scanner.generated.h"

class USphereComponent;
class UWidgetComponent;
class UW_ScannerSummary;

UCLASS()
class ALSEXTRAS_API AA_Scanner : public AActor
{
	GENERATED_BODY()

public:
	AA_Scanner();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMeshComponent* ScannerMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMeshComponent* ScannerLaser;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	USphereComponent* ScannerSphere;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UAudioComponent* AudioComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components|Sounds", meta = (AllowPrivateAccess = true))
	USoundBase* ScanningSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components|Sounds", meta = (AllowPrivateAccess = true))
	USoundBase* ScanningSuccessSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components|Sounds", meta = (AllowPrivateAccess = true))
	USoundBase* ScanningFailSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components|Visual", meta = (AllowPrivateAccess = true))
	UWidgetComponent* ScanningSummaryWidgetComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components|Visual", meta = (AllowPrivateAccess = true))
	UW_ScannerSummary* ScanningSummaryWidget;

	//Scanning Timeline
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UTimelineComponent* ScanningTimeline;

	UPROPERTY(EditDefaultsOnly, Category = "Components|Timeline")
	UCurveFloat* ScanningFloatCurve;

	FOnTimelineFloat ScanningProgressFunction;

	FOnTimelineEvent ScanningFinishedFunction;

	UFUNCTION()
	void ScanningTimelineProgress(float Value);

	UFUNCTION()
	void ScanningTimelineFinished();

	UFUNCTION()
	void OnScannerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnScannerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scanning")
	AActor* CurrentlyScannedActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scanning")
	TArray<AActor*> ScannedActors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scanning")
	uint8 bScannerEnabled : 1{true};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scanning", meta = (ClampMin = "0.0"))
	float ScanCooldown = 2.0f;

	FTimerHandle ScanCooldownTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scanning")
	TArray<FGameplayTag> AcceptedItemTypes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scanning")
	AActor* LastScannedItem;

	UFUNCTION(BlueprintCallable, Category = "Scanning")
	void ShowScanningSummary();

	UFUNCTION(BlueprintCallable, Category = "Scanning")
	bool CheckIfInAcceptedList(AActor* ActorToScan);

	UFUNCTION(BlueprintCallable, Category = "Scanning")
	bool CheckIfByOrderList(AActor* ActorToScan);
};
