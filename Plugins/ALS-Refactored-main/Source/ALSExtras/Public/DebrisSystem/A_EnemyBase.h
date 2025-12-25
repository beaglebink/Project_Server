#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_EnemyBase.generated.h"

class AA_Debris;

UCLASS()
class ALSEXTRAS_API AA_EnemyBase : public AActor
{
	GENERATED_BODY()

public:
	AA_EnemyBase();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	TSubclassOf<AA_Debris> DebrisClass;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debris Settings")
	TArray<UStaticMesh*> DebrisMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debris Settings")
	TArray<FTransform> DebrisLocationOffsets;
};
