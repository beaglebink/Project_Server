#pragma once

#include "Interfaces\I_WeaponInteraction.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_Rift.generated.h"

class UNiagaraComponent;
class UBoxComponent;

UCLASS()
class ALSEXTRAS_API AA_Rift : public AActor, public II_WeaponInteraction
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
	TArray<UBoxComponent*> FromLeftBoxes;

	UPROPERTY()
	TArray<UBoxComponent*> FromRightBoxes;

	UPROPERTY(EditAnywhere, Category = "Setup")
	FVector BoxSize = FVector(2.0f, 20.f, 20.f);
	
	virtual void HandleWeaponShot_Implementation(FHitResult& Hit)override;

	virtual void HandleTextFromWeapon_Implementation(const FText& TextCommand)override;
};
