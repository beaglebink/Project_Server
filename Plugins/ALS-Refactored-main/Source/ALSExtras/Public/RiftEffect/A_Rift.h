#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_Rift.generated.h"

class UNiagaraComponent;
class UBoxComponent;

UENUM(BlueprintType)
enum class ERiftSide : uint8
{
	Left,
	Right
};

USTRUCT(BlueprintType)
struct FRiftBoxData
{
	GENERATED_BODY()

	UPROPERTY()
	UBoxComponent* BoxComponent;

	UPROPERTY()
	int32 Index;

	UPROPERTY()
	ERiftSide Side;
};

UCLASS()
class ALSEXTRAS_API AA_Rift : public AActor
{
	GENERATED_BODY()

public:
	AA_Rift();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components", meta = (AllowPrivateAccess = true))
	UNiagaraComponent* NiagaraComp;

protected:
	UPROPERTY()
	TArray<FRiftBoxData> FromLeftBoxes;

	UPROPERTY()
	TArray<FRiftBoxData> FromRightBoxes;

	UPROPERTY(EditAnywhere, Category = "Setup")
	FVector BoxSize = FVector(2.0f, 20.f, 20.f);

	UFUNCTION()
	void OnBoxHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
