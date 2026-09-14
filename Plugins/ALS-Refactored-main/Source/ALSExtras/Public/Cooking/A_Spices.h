#pragma once

#include "CoreMinimal.h"
#include "PythonContainers/A_InteractableActor.h"
#include "Cooking/A_Dishes.h"
#include "A_Spices.generated.h"

UCLASS()
class ALSEXTRAS_API AA_Spices : public AA_InteractableActor
{
	GENERATED_BODY()

public:
	AA_Spices();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	virtual void Destroyed() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "CookingSettings")
	AA_Dishes* ParentDish;
};
