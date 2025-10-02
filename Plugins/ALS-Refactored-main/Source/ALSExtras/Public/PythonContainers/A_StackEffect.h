#pragma once

#include "PythonContainers/A_PythonContainer.h"
#include "Components/TimelineComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_StackEffect.generated.h"

class AA_ContainerNode;
class UBoxComponent;
class UTextRenderComponent;

UCLASS()
class ALSEXTRAS_API AA_StackEffect : public AA_PythonContainer
{
	GENERATED_BODY()
	
public:	
	AA_StackEffect();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void GetTextCommand(FText Command)override;
};
