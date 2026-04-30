#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SC_ItemPlacement.generated.h"

class USphereComponent;
class AA_DropZone;
class AA_InteractableActor;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FPSKITALSREFACTORED_API USC_ItemPlacement : public USceneComponent
{
	GENERATED_BODY()

public:
	USC_ItemPlacement();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemPlacement")
	USphereComponent* DropZoneSearcherSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemPlacement")
	USphereComponent* DropZoneCheckerSphere;

	UPROPERTY(BlueprintReadWrite, Category = "ItemPlacement")
	FName ItemName;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	AA_DropZone* ChoosenDropZone;

	UFUNCTION()
	void OnSearcherSphereOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSearcherSphereOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void OnCheckerSphereOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnCheckerSphereOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(BlueprintCallable, Category = "ItemPlacement")
	void GrabItemToDropZone(AA_InteractableActor* Item);

};
