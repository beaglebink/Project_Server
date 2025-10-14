#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_Book.generated.h"

UCLASS()
class ALSEXTRAS_API AA_Book : public AActor
{
	GENERATED_BODY()

public:
	AA_Book();

protected:
	virtual void OnCostruction(const FTransform& Transform)override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMeshComponent* StaticMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components|Property", meta = (AllowPrivateAccess = true))
	int32 BookGroupCode = -1;

	void OnMeshBeginOverlap(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
