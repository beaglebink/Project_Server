#pragma once

#include "CoreMinimal.h"
#include "PythonContainers/A_InteractableActor.h"
#include "A_Cookable.generated.h"

UCLASS()
class ALSEXTRAS_API AA_Cookable : public AA_InteractableActor
{
	GENERATED_BODY()
	
public:
	AA_Cookable();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	virtual void Destroyed() override;

	void HandleCutting_Implementation(UPARAM(ref)FHitResult& Hit, FRotator WeaponRotation);
};
