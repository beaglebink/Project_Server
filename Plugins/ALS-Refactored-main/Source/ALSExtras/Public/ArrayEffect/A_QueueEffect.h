#pragma once

#include "ArrayEffect/A_PythonContainer.h"
#include "Components/TimelineComponent.h"
#include "CoreMinimal.h"
#include "A_QueueEffect.generated.h"

class AA_ArrayNode;
class UBoxComponent;
class UTextRenderComponent;

UCLASS()
class ALSEXTRAS_API AA_QueueEffect : public AA_PythonContainer
{
	GENERATED_BODY()

public:
	AA_QueueEffect();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void GetTextCommand(FText Command)override;

public:
	virtual void AppendNode(FName VariableName = NAME_None)override;

private:
	void ContainerPop(int32 Index);

	void ContainerClear();

	void ContainerExtend();

private:
	bool ParseCommandToAppend(FText Command, FText& PrevName, FName& VariableName);

	bool ParseCommandToDelete(FText Command, FText& PrevName, int32& OutIndex, int32& OutLeftIndex, int32& OutRightIndex);
	
	bool ParseCommandToPop(FText Command, FText& PrevName, FName& VariableName, int32& Index);
};
