#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_Barrier.generated.h"

class UBoxComponent;

UCLASS()
class ALSEXTRAS_API AA_Barrier : public AActor
{
	GENERATED_BODY()
	
public:	
	AA_Barrier();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	class UBoxComponent* BoxComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UAudioComponent* AudioComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	FText BarrierPassword;

	UPROPERTY()
	UMaterialInstanceDynamic* MaterialInstanceDynamic;

	uint8 bIsOverlapping : 1{false};

	AActor* OverlappingActor{ nullptr };

	UFUNCTION()
	void OnBarrierBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnBarrierEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	UFUNCTION(BlueprintCallable, Category = "Setter")
	void GetTextCommand(FText Command);

	UFUNCTION(BlueprintCallable, Category = "Setter")
	void SetBarrierPassword(const FText& NewPassword);

	UFUNCTION(BlueprintCallable, Category = "Getter")
	FText GetBarrierPassword() const;
};
