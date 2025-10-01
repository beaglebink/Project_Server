#pragma once

#include "ArrayEffect/A_PythonContainer.h"
#include "Components/TimelineComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_StackEffect.generated.h"

class AA_ArrayNode;
class UBoxComponent;
class UTextRenderComponent;

UCLASS()
class ALSEXTRAS_API AA_StackEffect : public AA_PythonContainer
{
	GENERATED_BODY()
	
public:	
	AA_StackEffect();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void GetTextCommand(FText Command)override;

	virtual void AppendNode(FName VariableName = NAME_None)override;

private:
	bool ParseCommandToAppend(FText Command, FText& PrevName, FName& VariableName);

	bool ParseCommandToPop(FText Command, FText& PrevName, FName& VariableName, int32& Index);
};
